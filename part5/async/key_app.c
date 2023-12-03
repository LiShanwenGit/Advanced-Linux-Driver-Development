#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#define DEV_NAME "/dev/key"

int fd;

void signal_hook(int signum)
{
    printf("key press!\n");
}

int main(int argc, char **argv)
{
    unsigned char key_val;
    int ret;
    int Oflags;

    //信号绑定
    signal(SIGIO, signal_hook);

    fd = open(DEV_NAME, O_RDWR);
    if (fd < 0)
    {
        printf("can't open!\n");
        exit(-1);
    }
    printf("open OK, fd = 0x%x\n", fd);
    fcntl(fd, F_SETOWN, getpid()); //设置当前内核中filp owner
    Oflags = fcntl(fd, F_GETFL); //获取fd的状态
    fcntl(fd, F_SETFL, Oflags | FASYNC); //设置fd的状态，增加异步IO
    while (1)
    {
        //死循环，无限睡眠
        sleep(1000);
    }
    exit(0);
}
