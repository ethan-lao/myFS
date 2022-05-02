#include "bitmap.h"

// Error logging for THIS MODULE, helps differentiate from logging of other modules
// Prints errors and logging info to STDOUT
// Passes format strings and args to vprintf, basically a wrapper for printf
static void error_log(char *fmt, ...) {
#ifdef ERR_FLAG
    va_list args;
    va_start(args, fmt);
    
    printf("BITMAP : ");
    vprintf(fmt, args);
    printf("\n");

    va_end(args);
#endif
}

uint64_t findFirstFreeBlockAndSet() {
    for (int i = 0; i < NUMBLOCKS; i++) {
        if (!get_bit(i)) {
            set_bit(i);
            // fprintf(stderr, "returning bit %d\n", i);
            return i;
        }
    }
    
    error_log("Returning not found!");
    fprintf(stderr, "Out of space!!!\n");
    return -1;
}

void set_bit(int n) { 
    bitmap[WORD_OFFSET(n)] |= (1 << BIT_OFFSET(n));
}

void clear_bit(int n) {
    bitmap[WORD_OFFSET(n)] &= ~(1 << BIT_OFFSET(n)); 
}

int get_bit(int n) {
    word_t bit = bitmap[WORD_OFFSET(n)] & (1 << BIT_OFFSET(n));
    return bit != 0; 
}

void print_bitmap(int n) {
    for (int i = 0; i < n; i++) {
        if (get_bit(i)) {
            fprintf(stderr, "1");
        } else {
            fprintf(stderr, "0");
        }
    }
    fprintf(stderr, "\n");
}
