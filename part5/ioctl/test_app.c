#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEV_NAME "/dev/test"

#define IOCTL_READ   0
#define IOCTL_WRITE  1

int main (int argc, char **argv) 
{
    int data = 0;
    int fd = open(DEV_NAME, O_RDONLY);  // 打开设备文件
    if (fd < 0) {
        printf("can't open %s device file\n",DEV_NAME);
        return -1;
    }
    printf("write data:1234\n");
    data = 1234;
    ioctl(fd, IOCTL_WRITE, &data);
    data = 0;
    printf("read data: ");
    ioctl(fd, IOCTL_READ, &data);
    printf("%d\n", data);
    close(fd);  // 关闭设备文件
    return 0;
}
