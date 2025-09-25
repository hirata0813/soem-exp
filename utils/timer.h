#ifndef TIMER_H
#define TIMER_H

#include <x86intrin.h>
#include <stdint.h>
#include <time.h>

#define NSEC_MAX 999999999L

// for processing time 
extern int clock_index;
extern uint64_t clocks[10];

inline __attribute__((__always_inline__)) void get_clock_rdtsc(int index) {
  clocks[index] = __rdtsc();
}

void reset_clock_index(void);

// for timestamp (unix)
extern struct timespec unix_timestamps[10];
extern struct timespec tmp_timespec;

void init_base_time(void);
void save_unix_timestamp(int index);

#endif
