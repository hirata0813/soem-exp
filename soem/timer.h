#ifndef TIMER_H
#define TIMER_H

#include <x86intrin.h>
#include <stdint.h>

extern int clock_index;
extern uint64_t clocks[10];

inline __attribute__((__always_inline__)) void get_clock_rdtsc(int index) {
  clocks[index] = __rdtsc();
}

void reset_clock_index(void);

#endif
