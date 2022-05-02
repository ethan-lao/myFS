#ifndef BITMAP_H
#define BITMAP_H
/*
    Responsible for managing and manipulating the bitmap.
*/

#include <limits.h>    /* for CHAR_BIT */
#include <stdint.h>   /* for uint64_t */
#include "myfs_operations.h"

typedef uint32_t word_t;
enum { BITS_PER_WORD = sizeof(word_t) * CHAR_BIT };
#define WORD_OFFSET(b) ((b) / BITS_PER_WORD)
#define BIT_OFFSET(b)  ((b) % BITS_PER_WORD)
#define NUMWORDS ((NUMBLOCKS / BITS_PER_WORD) + 1)

word_t *bitmap;

/*
Find first free block in the disk, i.e, the first 0 bit in the bitmap.
*/
uint64_t findFirstFreeBlockAndSet();

void set_bit(int n);

void clear_bit(int n);

int get_bit(int n);

void print_bitmap(int n);

#endif