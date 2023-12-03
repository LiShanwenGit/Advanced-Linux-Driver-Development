#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* thread_func(void* arg) {
    int thread_num = *(int*)arg;
    printf("Hello from thread %d!\n", thread_num);
    pthread_exit(NULL);
}

int main() {
    pthread_t thread;
    int thread_num = 1;

    int ret = pthread_create(&thread, NULL, thread_func, &thread_num);
    if (ret != 0) {
        printf("Failed to create thread. Error code: %d\n", ret);
        return 1;
    }

    printf("Thread created successfully.\n");

    // 等待线程结束
    pthread_join(thread, NULL);

    return 0;
}
