#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    
    int a = 10, b = 20, result;
    
    asm volatile (
        "ADD %[result], %[a], %[b]"
        : [result] "=r" (result)
        : [a] "r" (a),
          [b] "r" (b)
    );
    printf("a=10, b=20\n");
    printf("a+b=%d\n",result);
    return 0;
}
