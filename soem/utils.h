#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <x86intrin.h>

#define NSEC_MAX 999999999U

// 送受信回数計測用
extern int global_send_cnt;
extern int global_recv_cnt;
extern int global_send_err_cnt;
extern int global_recv_timeout_cnt;

// タイムスタンプ計測用
typedef struct {
  struct timespec mono_ts; // モノトニックタイムスタンプ
  double unix_ns; // UNIX タイムスタンプ 
} UnixTimeStamp;

extern unsigned long long tsc0;
extern UnixTimeStamp ts_base;
extern UnixTimeStamp ts0;
extern UnixTimeStamp ts1;
extern UnixTimeStamp ts2;
extern UnixTimeStamp ts3;
extern double cpu_hz; // CPUクロック周波数

double calibrate_cpu_hz();
void init_base_ts();
void calc_unix_ts(UnixTimeStamp *ts);


// ラウンドトリップ計測用
// typedef struct {
//   struct timespec mono_ts; // モノトニックタイムスタンプ
//   struct timespec epoch_ts; // UNIX タイムスタンプ 
// } TimeStamp;

// extern struct timespec base_ts;
// extern TimeStamp start_ts;
// extern TimeStamp end_ts;

// uint32_t calc_comm_duration_ns();
// void get_base_eopoch_time();
// void get_timestamp(TimeStamp *ts);

// ログファイル用
void open_logfile(char *fmt, ...);
void close_logfile();
void logfile_printf(char *fmt, ...);


#endif