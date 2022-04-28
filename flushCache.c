#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

long x = 1, y = 4, z = 7, w = 13;

long simplerand(void) {
	long t = x;
	t ^= t << 11;
	t ^= t >> 8;
	x = y;
	y = z;
	z = w;
	w ^= w >> 19;
	w ^= t;
	return w;
}

int main() {
    int buffer_size = 32768 + 1024;
    char *buff = malloc(buffer_size);
    for (int i = 0; i < buffer_size; i++) {
        buff[i] = simplerand();
    }
    free(buff);
    return 0;
}
