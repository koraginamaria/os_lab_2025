#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <seed> <array_size>\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Child process: Starting sequential_min_max with seed=%s, array_size=%s\n", 
               argv[1], argv[2]);
        
        // Запускаем sequential_min_max с переданными аргументами
        char *args[] = {"./sequiential_min_max", argv[1], argv[2], NULL};
        execvp(args[0], args);
        
        // Если execvp вернул управление - произошла ошибка
        perror("execvp failed");
        exit(1);
    } else {
        // Родительский процесс
        int status;
        printf("Parent process: Waiting for child (PID: %d) to finish...\n", pid);
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Parent process: Child exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Parent process: Child terminated by signal %d\n", WTERMSIG(status));
        } else {
            printf("Parent process: Child process terminated abnormally\n");
        }
    }
    
    return 0;
}
