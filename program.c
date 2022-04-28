#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

long simplerand(void) {
  return rand();
}

// repeated writes
int test1(int size) {
  int bufferSize = 1 << size;
  int loops = 1 << 5;

  for (int i = 0; i < loops; i++) {
    int fd0 = open("foo", O_RDWR);
    char buf[bufferSize];
    for (int j = 0; j < bufferSize; j++) {
        buf[j] = (simplerand() % 74) + 48;
    }

    int nb0 = write(fd0, buf, bufferSize);
    close(fd0);
  }
}

// sequential writes
int test2(int size) {
  int bufferSize = 1 << size;
  int loops = 1 << 5;

  int fd0 = open("foo", O_RDWR);
  for (int i = 0; i < loops; i++) {
    char buf[bufferSize];
    for (int j = 0; j < bufferSize; j++) {
        buf[j] = (simplerand() % 74) + 48;
    }

    int nb0 = write(fd0, buf, bufferSize);
  }
  close(fd0);
}

// sequential reads
int test3(int reads) {
  // assume foo is of size 1 mib
  int numBytes = 33554432;
  int bufferSize = 100;
  unsigned long loops = 2 << reads;

  int fd0 = open("foo", O_RDWR);
  lseek(fd0, (simplerand() % (numBytes - (loops * bufferSize))), SEEK_SET);
  for (int i = 0; i < loops; i++) {
    char buf[bufferSize];
    int nb0 = read(fd0, buf, 100);
    // printf("%s\n", buf);
  }
  close(fd0);
}

// random reads
int test4(int reads) {
  // assume foo is of size 1 mib
  int numBytes = 33554432;
  int bufferSize = 100;
  unsigned long loops = 2 << reads;

  int fd0 = open("foo", O_RDWR);
  for (int i = 0; i < loops; i++) {
    lseek(fd0, (simplerand() % (numBytes - (bufferSize + 1))), SEEK_SET);
    char buf[bufferSize];
    int nb0 = read(fd0, buf, 100);
    // printf("%s\n", buf);
  }
  close(fd0);
}

int main(int argc, char *argv[]) {
  srand(time(NULL));

  int test = atoi(argv[1]);
  int size = atoi(argv[2]);

  printf("Test: %d\n", test);
  printf("Size: %d\n", size);

  if (test == 1) {
    test1(size);
  } else if (test == 2) {
    test2(size);
  } else if (test == 3) {
    test3(size);
  } else if (test == 4) {
    test4(size);
  }

  return 0;
}