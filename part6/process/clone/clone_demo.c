#include <stdio.h>
#include <malloc.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#define _SCHED_H 1
#define __USE_GNU 1
#include <bits/sched.h>
#include <stdlib.h>

int num = 10;
#define STACK_SIZE 65536

int child_func(void *arg) {
    sleep(1);
    printf("Child process\n");
    printf("num = %d\n", num);
    return 0;
}

int main() {
    char *stack;
    stack = malloc(STACK_SIZE);
    if (stack == NULL) {
        printf("malloc error\n");
        exit(1);
    }

    pid_t pid = clone(child_func, stack + STACK_SIZE, CLONE_VM | SIGCHLD, NULL);
    if (pid == -1) {
        printf("clone error");
        exit(1);
    }
    num = 5;
    printf("Parent process \n");
    sleep(5);

    return 0;
}
