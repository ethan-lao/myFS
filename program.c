#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>

// Simple, fast random number generator, here so we can observe it using profiler
// long x = 1, y = 4, z = 7, w = 13;

long simplerand(void) {
// long t = x;
// t ^= t << 11;
// t ^= t >> 8;
// x = y;
// y = z;
// z = w;
// w ^= w >> 19;
// w ^= t;
  
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





  // 10: 1 kib

  // FUSE
  // real    0m48.561s
  // user    0m0.000s
  // sys     0m0.017s

  // NFS
  // real    0m1.652s
  // user    0m0.015s
  // sys     0m0.000s







  // 16: 64 kib

  // FUSE
  // real    0m53.433s
  // user    0m0.064s
  // sys     0m0.027s

  // NFS
  // real    0m13.249s
  // user    0m0.061s
  // sys     0m0.001s


  // 18: 256 kib

  // FUSE
  // real    0m59.056s
  // user    0m0.172s
  // sys     0m0.034s

  // NFS
  // real    0m13.007s
  // user    0m0.198s
  // sys     0m0.033s



  // 19: .5 mib

  // FUSE
  // real    1m20.327s
  // user    0m0.334s
  // sys     0m0.073s

  // NFS
  // real    0m16.620s
  // user    0m0.231s
  // sys     0m0.068s






  // 20: 1048576 bytes (1 mib)

  // FUSE
  // real    1m40.275s
  // user    0m0.470s
  // sys     0m0.089s

  // NFS
  // real    0m27.472s
  // user    0m0.502s
  // sys     0m0.074s
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

  // 10: 32 kib

  // FUSE
  // real	0m2.193s
  // user	0m0.000s
  // sys	0m0.004s


  // NFS
  // real    0m0.189s
  // user    0m0.000s
  // sys     0m0.005s






  // 16: 2 mib

  // FUSE
  // real	0m2.739s
  // user	0m0.032s
  // sys	0m0.028s

  // NFS
  // real    0m2.585s
  // user    0m0.024s
  // sys     0m0.004s





  // 18: 8 mib

  // FUSE
  // real	0m2.383s
  // user	0m0.080s
  // sys	0m0.014s

  // NFS
  // real    0m17.894s
  // user    0m0.145s
  // sys     0m0.062s






  // 19: 16 mib

  // FUSE
  // real	0m2.093s
  // user	0m0.110s
  // sys	0m0.025s

  // NFS
  // real    0m17.452s
  // user    0m0.340s
  // sys     0m0.119s






  // 20: 33554432 bytes (32 mib)

  // FUSE
  // real	0m2.110s
  // user	0m0.182s
  // sys	0m0.044s

  // NFS
  // real    0m28.864s
  // user    0m0.673s
  // sys     0m0.265s
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




  // 20

  // FUSE
  // real	0m3.262s
  // user	0m0.634s
  // sys	0m0.595s

  // NFS
  // real	0m1.319s
  // user	0m0.658s
  // sys	0m0.546s



  // 21

  // FUSE
  // real	0m4.755s
  // user	0m1.349s
  // sys	0m1.106s

  // NFS
  // real	0m2.421s
  // user	0m1.360s
  // sys	0m1.001s



  // 22

  // FUSE
  // real	0m7.411s
  // user	0m2.519s
  // sys	0m2.228s

  // NFS
  // real	0m4.804s
  // user	0m2.509s
  // sys	0m2.233s


  // 23

  // FUSE
  // real	0m11.402s
  // user	0m5.088s
  // sys	0m4.322s

  // NFS
  // real	0m9.993s
  // user	0m5.411s
  // sys	0m4.511s



  // 24

  // FUSE
  // real	0m23.169s
  // user	0m9.971s
  // sys	0m8.837s

  // NFS
  // real	0m19.297s
  // user	0m10.458s
  // sys	0m8.778s



  // 25

  // FUSE
  // real	0m42.186s
  // user	0m20.243s
  // sys	0m17.426s

  // NFS
  // real	0m39.155s
  // user	0m20.864s
  // sys	0m17.924s

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


  // 6

  // FUSE
  // real	0m1.718s
  // user	0m0.006s
  // sys	0m0.001s

  // NFS
  // real	0m1.703s
  // user	0m0.007s
  // sys	0m0.006s


  // 7

  // FUSE
  // real	0m3.245s
  // user	0m0.000s
  // sys	0m0.012s

  // NFS
  // real	0m7.670s
  // user	0m0.000s
  // sys	0m0.032s



  // 8

  // FUSE
  // real	0m1.851s
  // user	0m0.000s
  // sys	0m0.022s

  // NFS
  // real	0m12.972s
  // user	0m0.017s
  // sys	0m0.044s



  // 9

  // FUSE
  // real	0m1.772s
  // user	0m0.000s
  // sys	0m0.029s

  // NFS
  // real	0m23.517s
  // user	0m0.014s
  // sys	0m0.099s



  // 10

  // FUSE
  // real	0m1.873s
  // user	0m0.000s
  // sys	0m0.063s

  // NFS
  // real	0m49.982s
  // user	0m0.010s
  // sys	0m0.208s



  // 11

  // FUSE
  // real	0m2.105s
  // user	0m0.008s
  // sys	0m0.072s

  // NFS
  // real	1m7.617s
  // user	0m0.023s
  // sys	0m0.366s




}

int
main(int argc, char *argv[]) {
  // int fd0 = open("foo", O_RDWR);
  // int fd1 = open("foo", O_RDONLY);
  // char buf[100];
  // buf[0] = 9;
  // buf[1] = 81;
  // buf[2] = 'A';
  // buf[3] = 'q';
  // buf[4] = '0';
  // int nb0 = write(fd0, buf, 100);
  // int nb1;
  // close(fd0);
  // nb1 = read(fd1, buf, 100);
  // assert(buf[0] == 9);
  // assert(buf[1] == 81);
  // assert(buf[2] == 'A');
  // assert(buf[3] == 'q');
  // assert(buf[4] == '0');
  // close(fd1);
  // printf("Wrote %d, then %d bytes\n", nb0, nb1);
  // return 0;

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