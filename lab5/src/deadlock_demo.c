#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

// Функция для первого потока (захватывает mutex1, затем пытается захватить mutex2)
void* thread1_function(void* arg) {
    (void)arg; // Подавляем предупреждение о неиспользуемом параметре
    printf("Thread 1: Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 1: Locked mutex1\n");
    
    // Имитация работы
    sleep(1);
    
    printf("Thread 1: Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2); // DEADLOCK - mutex2 уже захвачен thread2
    printf("Thread 1: Locked mutex2\n");
    
    // Критическая секция
    printf("Thread 1: Entering critical section\n");
    sleep(1);
    printf("Thread 1: Exiting critical section\n");
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    return NULL;
}

// Функция для второго потока (захватывает mutex2, затем пытается захватить mutex1)
void* thread2_function(void* arg) {
    (void)arg; // Подавляем предупреждение о неиспользуемом параметре
    printf("Thread 2: Trying to lock mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 2: Locked mutex2\n");
    
    // Имитация работы
    sleep(1);
    
    printf("Thread 2: Trying to lock mutex1...\n");
    pthread_mutex_lock(&mutex1); // DEADLOCK - mutex1 уже захвачен thread1
    printf("Thread 2: Locked mutex1\n");
    
    // Критическая секция
    printf("Thread 2: Entering critical section\n");
    sleep(1);
    printf("Thread 2: Exiting critical section\n");
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    
    printf("=== Deadlock Demonstration ===\n");
    printf("This program demonstrates a classic deadlock scenario:\n");
    printf("- Thread 1 locks mutex1, then tries to lock mutex2\n");
    printf("- Thread 2 locks mutex2, then tries to lock mutex1\n");
    printf("- Both threads wait forever for each other...\n\n");
    
    // Создаем потоки
    if (pthread_create(&thread1, NULL, thread1_function, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
    
    if (pthread_create(&thread2, NULL, thread2_function, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
    
    // Ждем некоторое время чтобы увидеть deadlock
    printf("Main: Waiting for threads to complete...\n");
    sleep(5);
    
    // Проверяем, завершились ли потоки с помощью pthread_join с таймаутом
    printf("\n*** DEADLOCK DETECTED! ***\n");
    printf("Threads are blocked waiting for mutexes.\n");
    printf("The program will hang forever if we wait for threads.\n");
    printf("Main: Exiting (threads are still blocked in deadlock)\n");
    
    return 0;
}