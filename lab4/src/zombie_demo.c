
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main() {
    printf("=== Демонстрация зомби процессов ===\n\n");
    
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс: PID = %d, PPID = %d\n", getpid(), getppid());
        printf("Дочерний процесс: Завершаю работу через 5 секунд...\n");
        sleep(5);
        printf("Дочерний процесс: Завершаюсь!\n");
        exit(42);
    } else {
        // Родительский процесс
        printf("Родительский процесс: PID = %d\n", getpid());
        printf("Родительский процесс: Создал дочерний процесс с PID = %d\n", pid);
        printf("Родительский процесс: НЕ вызываю wait() - дочерний процесс станет зомби!\n");
        printf("Родительский процесс: Спим 30 секунд...\n\n");
        
        // Выводим информацию о процессах каждые 3 секунды
        for (int i = 0; i < 10; i++) {
            sleep(3);
            printf("Прошло %d секунд. Выполните в другом терминале: 'ps aux | grep %d'\n", 
                   (i+1)*3, pid);
            printf("Вы увидите дочерний процесс в состоянии 'Z' (зомби)\n\n");
        }
        
        printf("Родительский процесс: Теперь вызываю wait() чтобы убрать зомби...\n");
        int status;
        waitpid(pid, &status, 0);
        printf("Родительский процесс: Дочерний процесс завершился со статусом: %d\n", 
               WEXITSTATUS(status));
        printf("Родительский процесс: Зомби убран!\n");
    }
    
    return 0;
}
