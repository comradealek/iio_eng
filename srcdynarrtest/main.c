#include <dynarr_3.h>
#include <stdio.h>
#define __USE_POSIX199309
#include <time.h>


typedef unsigned int uint;
typedef unsigned long long int uint64;
const int testcount = 100;

int main(void) {
    unsigned int value;
    byteArr * array;
    struct timespec beginning;
    struct timespec end;

    uint64 macro_accum = 0;
    for (int i = 1; i <= testcount; i++) {
        fprintf(stdout, "Test round %d\n", i);

        value = 0;
        dynarr_init_m(uint, array);
        clock_gettime(CLOCK_MONOTONIC, &beginning);
        while(value < 0x7fff8) {
            dynarr_push_m(array, &value, sizeof(uint));
            value++;
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        fprintf(stdout, "  Time to process 2GB of integers using macros: %llu sec, %llu nsec\n", end.tv_sec - beginning.tv_sec, end.tv_nsec - beginning.tv_nsec);
        macro_accum += (end.tv_nsec - beginning.tv_nsec);
        memset(array, 0, array->t.cap + sizeof(Tracker));
        free(array);
    }

    uint64 func_accum = 0;
    for (int i = 1; i <= testcount; i++) {
        fprintf(stdout, "Test round %d\n", i);
        value = 0;
        array = dynarr_init(sizeof(uint));
        clock_gettime(CLOCK_MONOTONIC, &beginning);
        while(value < 0x7fff8) {
            array = dynarr_push(array, &value, sizeof(uint));
            value++;
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        fprintf(stdout, "  Time to process 2GB of integers using functions: %llu sec, %llu nsec\n", end.tv_sec - beginning.tv_sec, end.tv_nsec - beginning.tv_nsec);
        func_accum += (end.tv_nsec - beginning.tv_nsec);
        memset(array, 0, array->t.cap + sizeof(Tracker));
        free(array);
    }
    fprintf(stdout, "average for macro: %llu\n", macro_accum/testcount);
    fprintf(stdout, "average for functions: %llu\n", func_accum/testcount);
}