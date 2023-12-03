#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    pid_t pid = vfork();
	assert(pid != -1);
	if(pid == 0) {
        printf("child PID: %d\n", getpid());
        printf("child task!\n");
        exit(0);
    }
    else {
        printf("parent PID: %d\n", getpid());
        printf("parent task!\n");
    }
    return 0;
}
