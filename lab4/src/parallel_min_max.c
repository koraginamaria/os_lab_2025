#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

volatile sig_atomic_t timeout_occurred = 0;

void timeout_handler(int sig) {
    timeout_occurred = 1;
}

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    int timeout = 0; // 0 means no timeout
    bool with_files = false;

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"by_files", no_argument, 0, 'f'},
            {"timeout", required_argument, 0, 't'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "ft:", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        seed = atoi(optarg);
                        if (seed <= 0) {
                            printf("seed must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 1:
                        array_size = atoi(optarg);
                        if (array_size <= 0) {
                            printf("array_size must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 2:
                        pnum = atoi(optarg);
                        if (pnum <= 0) {
                            printf("pnum must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 3:
                        with_files = true;
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case 'f':
                with_files = true;
                break;
            case 't':
                timeout = atoi(optarg);
                if (timeout <= 0) {
                    printf("timeout must be a positive number\n");
                    return 1;
                }
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (optind < argc) {
        printf("Has at least one no option argument\n");
        return 1;
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--by_files] [--timeout \"seconds\"]\n",
               argv[0]);
        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);
    
    // Создаем pipes для каждого процесса, если не используем файлы
    int pipes[2 * pnum];
    if (!with_files) {
        for (int i = 0; i < pnum; i++) {
            if (pipe(pipes + i * 2) < 0) {
                printf("Pipe failed!\n");
                return 1;
            }
        }
    }

    int active_child_processes = 0;
    pid_t child_pids[pnum];

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Устанавливаем обработчик сигнала таймаута
    if (timeout > 0) {
        signal(SIGALRM, timeout_handler);
        alarm(timeout);
    }

    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid >= 0) {
            // successful fork
            active_child_processes += 1;
            child_pids[i] = child_pid;
            if (child_pid == 0) {
                // child process - отключаем обработчик сигнала
                if (timeout > 0) {
                    signal(SIGALRM, SIG_DFL);
                }
                
                // Вычисляем диапазон для текущего процесса
                int chunk_size = array_size / pnum;
                int begin = i * chunk_size;
                int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;
                
                struct MinMax local_min_max = GetMinMax(array, begin, end);
                
                if (with_files) {
                    // use files here
                    char filename[20];
                    sprintf(filename, "min_max_%d.txt", i);
                    FILE *file = fopen(filename, "w");
                    if (file != NULL) {
                        fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
                        fclose(file);
                    }
                } else {
                    // use pipe here
                    close(pipes[i * 2]); // закрываем чтение
                    write(pipes[i * 2 + 1], &local_min_max.min, sizeof(int));
                    write(pipes[i * 2 + 1], &local_min_max.max, sizeof(int));
                    close(pipes[i * 2 + 1]);
                }
                free(array);
                return 0;
            }
        } else {
            printf("Fork failed!\n");
            return 1;
        }
    }

    // Закрываем ненужные дескрипторы в родительском процессе
    if (!with_files) {
        for (int i = 0; i < pnum; i++) {
            close(pipes[i * 2 + 1]); // закрываем запись
        }
    }

    // Неблокирующее ожидание с проверкой таймаута
    while (active_child_processes > 0) {
        int status;
        pid_t finished_pid = waitpid(-1, &status, WNOHANG);
        
        if (finished_pid > 0) {
            // Один из дочерних процессов завершился
            active_child_processes -= 1;
            printf("Child process %d finished\n", finished_pid);
        } else if (finished_pid == 0) {
            // Дочерние процессы еще работают, проверяем таймаут
            if (timeout_occurred) {
                printf("Timeout occurred! Sending SIGKILL to all child processes\n");
                for (int i = 0; i < pnum; i++) {
                    if (child_pids[i] > 0) {
                        kill(child_pids[i], SIGKILL);
                        printf("Sent SIGKILL to child process %d\n", child_pids[i]);
                    }
                }
                break;
            }
            // Ждем немного перед следующей проверкой
            usleep(100000); // 100ms
        } else {
            // Ошибка waitpid
            perror("waitpid failed");
            break;
        }
    }

    // Если был таймаут, дожидаемся завершения убитых процессов
    if (timeout_occurred) {
        while (active_child_processes > 0) {
            waitpid(-1, NULL, 0);
            active_child_processes -= 1;
        }
    }

    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    for (int i = 0; i < pnum; i++) {
        int min = INT_MAX;
        int max = INT_MIN;

        if (timeout_occurred) {
            // При таймауте результаты могут быть неполными
            min = 0;
            max = 0;
        } else if (with_files) {
            // read from files
            char filename[20];
            sprintf(filename, "min_max_%d.txt", i);
            FILE *file = fopen(filename, "r");
            if (file != NULL) {
                fscanf(file, "%d %d", &min, &max);
                fclose(file);
                remove(filename); // удаляем временный файл
            }
        } else {
            // read from pipes
            read(pipes[i * 2], &min, sizeof(int));
            read(pipes[i * 2], &max, sizeof(int));
            close(pipes[i * 2]);
        }

        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);

    if (timeout_occurred) {
        printf("Execution terminated by timeout after %d seconds\n", timeout);
        printf("Partial results - Min: %d, Max: %d\n", min_max.min, min_max.max);
    } else {
        printf("Min: %d\n", min_max.min);
        printf("Max: %d\n", min_max.max);
    }
    printf("Elapsed time: %fms\n", elapsed_time);
    fflush(NULL);
    return 0;
}