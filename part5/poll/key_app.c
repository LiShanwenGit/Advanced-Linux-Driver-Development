#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>

#define DEV_NAME "/dev/key"

int main (int argc, char **argv) 
{
    int fd = open(DEV_NAME, O_RDONLY | O_NONBLOCK);  // 打开设备文件
    if (fd < 0) {
        printf("can't open %s device file\n",DEV_NAME);
        return -1;
    }
    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = POLLIN;
    while (1) {
        int ret = poll(fds, 1, 1000);  // 等待事件发生
        if (ret < 0) {
            printf("poll error\n");
            break;
        }
        else if (fds[0].revents & POLLIN) {  // 检查是否有可读事件
            int key;
            ssize_t len = read(fd, &key, sizeof(key));  // 读取按键值
            if (len < 0) {
                printf("read /dev/key failed\n");
                break;
            }
            printf("key press: %d\n", key);
        }
        else if (ret == 0){
            printf("time out\n");
        }
    }
    close(fd);  // 关闭设备文件
    return 0;
}