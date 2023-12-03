#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

int main(int argc, char **argv)
{
    int num=10;
    pid_t pid = fork();
	assert(pid != -1);
	if(pid == 0) {
        sleep(2);
        printf("child PID: %d\n", getpid());
        printf("child: num = %d\n", num);
    }
    else {
        num = 5;
        printf("parent PID: %d\n", getpid());
        printf("parent: num = %d\n", num);
    }
    return 0;
}
