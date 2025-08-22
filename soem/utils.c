#include "utils.h"

// 送受信計測用
int global_send_cnt = 0;
int global_recv_cnt = 0;
int global_send_err_cnt  = 0;
int global_recv_timeout_cnt = 0;

// タイムスタンプ計測用
UnixTimeStamp ts_base;
UnixTimeStamp ts0;
UnixTimeStamp ts1;
UnixTimeStamp ts2;
UnixTimeStamp ts3;
double cpu_hz; // CPUクロック周波数

// RTT 計測用
// struct timespec base_ts;
// TimeStamp start_ts;
// TimeStamp end_ts;

// ログファイル用
FILE *log_fp;
char log_name[100];

double calibrate_cpu_hz() {
  struct timespec ts1, ts2;
  unsigned long long tsc1, tsc2;

  clock_gettime(CLOCK_MONOTONIC_RAW, &ts1);
  tsc1 = __rdtsc();

  sleep(1); // 1秒待つ

  clock_gettime(CLOCK_MONOTONIC_RAW, &ts2);
  tsc2 = __rdtsc();

  double ns = (ts2.tv_sec - ts1.tv_sec) * 1e9 +
              (ts2.tv_nsec - ts1.tv_nsec);
  double cpu_hz = (tsc2 - tsc1) / (ns * 1e-9);

  return cpu_hz;
}

void init_base_ts() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts_base.mono_ts);
  clock_gettime(CLOCK_REALTIME, &ts);

  ts_base.unix_ns = ts.tv_sec * 1e9 + ts.tv_nsec;

  // RTT 計測用
  // get_base_eopoch_time();
  // get_timestamp(&start_ts);
  // get_timestamp(&end_ts);
}

void calc_unix_ts(UnixTimeStamp *ts) {
  clock_gettime(CLOCK_MONOTONIC, &ts->mono_ts);

  double diff_sec = ts->mono_ts.tv_sec - ts_base.mono_ts.tv_sec;
  double diff_nsec = ts->mono_ts.tv_nsec - ts_base.mono_ts.tv_nsec;
  if (diff_nsec < 0) {
    diff_sec -= 1;
    diff_nsec += 1e9;
  }

  ts->unix_ns = ts_base.unix_ns + diff_sec * 1e9 + diff_nsec;

  // RTT 計測用
  // get_timestamp(ts);
  // ts->epoch_ts = base_ts;
  // ts->mono_ts = start_ts.mono_ts;
  // ts->unix_ts = start_ts.epoch_ts;
}


// uint32_t calc_comm_duration_ns() {
//   uint32_t ns;
//   // printf("\nstart: %ld.%09ld\n", start_ts.tv_sec, start_ts.tv_nsec);
//   // printf("\nend  : %ld.%09ld\n", end_ts.tv_sec, end_ts.tv_nsec);

//   ns = end_ts.mono_ts.tv_nsec - start_ts.mono_ts.tv_nsec;
//   if (end_ts.mono_ts.tv_sec > start_ts.mono_ts.tv_sec) {
//     ns = NSEC_MAX + ns;
//   }

//   return ns;
// }

// void get_base_eopoch_time() {
//   // Get the current time in seconds and nanoseconds
//   clock_gettime(CLOCK_REALTIME, &base_ts);
// }


// void get_timestamp(TimeStamp *ts) {
//   clock_gettime(CLOCK_REALTIME, &ts->epoch_ts);
//   clock_gettime(CLOCK_MONOTONIC, &ts->mono_ts);
// }


void open_logfile(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsprintf(log_name, fmt, args);
  va_end(args);

  log_fp = fopen(log_name, "w");
}

void close_logfile() {
  fclose(log_fp);
}

void logfile_printf(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(log_fp, fmt, args);
  va_end(args);
}
