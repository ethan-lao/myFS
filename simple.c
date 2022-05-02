#include <stdio.h>

// // repeated writes
// int test1(int size) {
//   int bufferSize = 1 << size;
//   int loops = 1 << 5;

//   for (int i = 0; i < loops; i++) {
//     int fd0 = open("foo", O_RDWR);
//     char buf[bufferSize];
//     for (int j = 0; j < bufferSize; j++) {
//         buf[j] = (simplerand() % 74) + 48;
//     }

//     int nb0 = write(fd0, buf, bufferSize);
//     close(fd0);
//   }
// }

int main(int argc, char *argv[]) {
    printf("Test\n");


  
}