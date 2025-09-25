#include "log.h"

// 送受信計測用
int global_send_cnt = 0;
int global_recv_cnt = 0;
int global_send_err_cnt  = 0;
int global_recv_timeout_cnt = 0;

// ppoll 回数計測用
int global_ppoll_cnt = 0;

// ログファイル用
FILE *log_fp;
char log_name[100];

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
