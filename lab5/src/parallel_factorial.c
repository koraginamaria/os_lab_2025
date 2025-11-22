#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <stdbool.h>

// Глобальные переменные
int k = 0;
int pnum = 0;
int mod = 0;
long long result = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Структура для передачи данных в поток
struct ThreadData {
    int thread_id;
    int start;
    int end;
};

// Функция для вычисления частичного произведения
void* compute_partial_factorial(void* arg) {
    struct ThreadData* data = (struct ThreadData*)arg;
    long long partial_result = 1;
    
    printf("Thread %d: computing from %d to %d\n", 
           data->thread_id, data->start, data->end);
    
    for (int i = data->start; i <= data->end; i++) {
        partial_result = (partial_result * i) % mod;
    }
    
    // Защищаем доступ к глобальному результату с помощью мьютекса
    pthread_mutex_lock(&mutex);
    result = (result * partial_result) % mod;
    pthread_mutex_unlock(&mutex);
    
    printf("Thread %d: partial result = %lld\n", 
           data->thread_id, partial_result);
    
    return NULL;
}

int main(int argc, char *argv[]) {
    // Обработка аргументов командной строки
    static struct option options[] = {
        {"k", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"mod", required_argument, 0, 0},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    while (true) {
        int c = getopt_long(argc, argv, "", options, &option_index);
        if (c == -1) break;
        
        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        k = atoi(optarg);
                        break;
                    case 1:
                        pnum = atoi(optarg);
                        break;
                    case 2:
                        mod = atoi(optarg);
                        break;
                    default:
                        printf("Unknown option\n");
                        return 1;
                }
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }
    
    // Проверка входных параметров
    if (k <= 0 || pnum <= 0 || mod <= 0) {
        printf("Usage: %s --k <number> --pnum <threads> --mod <modulus>\n", argv[0]);
        printf("All parameters must be positive integers\n");
        return 1;
    }
    
    if (pnum > k) {
        pnum = k; // Нельзя иметь больше потоков чем чисел для умножения
    }
    
    printf("Computing %d! mod %d using %d threads\n", k, mod, pnum);
    
    pthread_t threads[pnum];
    struct ThreadData thread_data[pnum];
    
    // Распределение работы между потоками
    int numbers_per_thread = k / pnum;
    int remainder = k % pnum;
    int current_start = 1;
    
    // Создание потоков
    for (int i = 0; i < pnum; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].start = current_start;
        
        // Распределяем остаток по первым потокам
        int numbers_for_this_thread = numbers_per_thread;
        if (i < remainder) {
            numbers_for_this_thread++;
        }
        
        thread_data[i].end = current_start + numbers_for_this_thread - 1;
        current_start = thread_data[i].end + 1;
        
        if (pthread_create(&threads[i], NULL, compute_partial_factorial, &thread_data[i])) {
            printf("Error: pthread_create failed!\n");
            return 1;
        }
    }
    
    // Ожидание завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Уничтожение мьютекса
    pthread_mutex_destroy(&mutex);
    
    printf("\nFinal result: %d! mod %d = %lld\n", k, mod, result);
    
    return 0;
}
