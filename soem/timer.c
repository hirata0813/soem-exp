#include "timer.h"

int clock_index = 0;
uint64_t clocks[10];

void reset_clock_index(void) {
  clock_index = 0;
}
