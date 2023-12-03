#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define DEV_NAME "/dev/key"

int main(int argc, char **argv)
{
    unsigned int keyval;
    int fd = 0;
    /* 打开设备文件 */
    fd = open(DEV_NAME, O_RDWR);
    if (fd < 0) {
        printf("can't open %s device file\n",DEV_NAME);
        return -1;
    }
    while(1)
    {
        read(fd, &keyval,sizeof(keyval));
        printf("key_value = %d\n",keyval);
    }
    return 0;
}
