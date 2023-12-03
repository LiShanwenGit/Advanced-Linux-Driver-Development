#include <sys/epoll.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define DEV_NAME "/dev/key"
#define MAX_EVENTS 5


int main(int argc, char *argv[]) {
    int epoll_fd, nfds;
    struct epoll_event ev, events[MAX_EVENTS];
    int value;

    // 创建epoll描述符
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return 1;
    }

    // 打开文件并添加到epoll描述符
    int fd = open(DEV_NAME, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    ev.events = EPOLLIN; // 监听输入事件
    ev.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl");
        return 1;
    }

    // 等待事件发生
    nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
        perror("epoll_wait");
        return 1;
    }

    // 处理事件
    for (int i = 0; i < nfds; ++i) {
        printf("Reading file: %d\n", events[i].data.fd);
        if(read(events[i].data.fd, &value, 4) > 0) {
            printf("value = %d\n", value);
        }
    }

    // 关闭文件和epoll描述符
    close(fd);
    close(epoll_fd);
    return 0;
}
