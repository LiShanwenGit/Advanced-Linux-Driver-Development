#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/epoll.h>

#define DEV_NAME "/sys/sysfs_notify/notify"

int main (int argc, char **argv) 
{
    struct pollfd fds;
    int fd, ret;
    int buf[5] = {[0 ... 4]=0};
    fd = open(DEV_NAME, O_RDWR | O_NONBLOCK);
    
    if (fd < 0) {
        printf("%s open error\n", DEV_NAME);
        return 1;
    }
    read(fd, buf, sizeof(buf));

    fds.fd = fd;
    fds.events = POLLPRI;

    printf("waiting for events...\n");
    while (1) {
        ret = poll(&fds, 1, -1);
        if (ret == -1) {
            printf("poll error\n");
            break;
        }
		if (ret < 0)
			break;
        ret = read(fd, buf, sizeof(buf));
		if (ret < 0)
			break;
		ret = lseek(fds.fd, 0, SEEK_SET);
		if (ret < 0) {
			printf("lseek failed (%d)\n", ret);
			break;
		}
        if (fds.revents & POLLPRI) {
            printf("key press = %s", buf);
        }
    }
    close(fd);  // 关闭设备文件
    return 0;
}
