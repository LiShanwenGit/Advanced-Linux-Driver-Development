#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int fd;
    char *map;
    // 打开文件
    fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("open failed\n");
        return 1;
    }
    // 创建内存映射, 注意：这里映射一个内核PAGE内存，即4096字节
    map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap failed\n");
        return 1;
    }
    // 对映射的文件进行操作
    printf("data: %s\n", map);
    // 修改映射的文件内容
    map[0] = 'H';
    map[1] = 'e';
    map[2] = 'l';
    map[3] = 'l';
    map[4] = 'o';
    // 解除内存映射
    if (munmap(map, 4096) == -1) {
        perror("munmap failed\n");
        return 1;
    }
    // 关闭文件
    if (close(fd) == -1) {
        perror("close failed\n");
        return 1;
    }
    return 0;
}
