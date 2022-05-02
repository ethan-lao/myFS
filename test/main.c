#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mman.h>

#define size 1024*1024*1024

int main(int argc, char *argv[]) {

    // int fd = open("./backing.txt", O_RDWR);
    // uint8_t *disk = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    uint8_t *disk = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    memset(disk, 0, size);

    printf("%d\n", *(disk));
    printf("%d\n", *(disk + size - 20));
}