#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>

#define DEV_NAME "/dev/key"

int main (int argc, char **argv) 
{
    int fd = open(DEV_NAME, O_RDONLY | O_NONBLOCK);  // 打开设备文件
    if (fd < 0) {
        printf("can't open %s device file\n",DEV_NAME);
        return -1;
    }
    // 设置文件描述符集合
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    while (1) {
        // 使用select监听设备文件的可读事件
        int ret = select(fd + 1, &fds, NULL, NULL, NULL);
        if (ret == -1) {
            printf("select error\n");
            return -1;
        }
        // 检查是否有数据可读
        if (FD_ISSET(fd, &fds)) {
            // 读取数据
            int key;
            ssize_t bytesRead = read(fd, &key, sizeof(key));
            if (bytesRead == -1) {
                printf("read data error\n");
                return -1;
            }
            // 处理数据
            printf("key press: %d\n", key);
        }
    }
    close(fd);  // 关闭设备文件
    return 0;
}