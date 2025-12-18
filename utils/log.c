#include "log.h"
#include "../pt-prio/common.h"
#include "string.h"
const unsigned long long CPU_FREQ_HZ = 3500000000UL;

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

// ログファイル出力用関数
// io_start,io_end,disturb_num,repeat_cnt を用いて,io_end[n] - io_start[n] の値を求めログファイルに出力
void logfile_output(char *fname) {
  FILE *fp = fopen(fname,"a");
  if (fp == NULL) {
      perror("fopen failed");
      return;
  }
  double elapsed = 0.0;
  //printf("logfile_output: repeat_cnt=%d,disturb_num=%d\n", repeat_cnt, disturb_num);

  for (int i = 0; i < repeat_cnt; i++) {
        elapsed = (io_end[i] - io_start[i]) / (double)CPU_FREQ_HZ;
        fprintf(fp, "%d,%.9f\n", disturb_num, elapsed);
  }
  fclose(fp);
}

// ループ情報出力用関数
// あるn回目のループについて，そのループにかかった時間，ppoll が完了したか，そのループは何回目のポーリングか，の3つの情報を出力
void loop_info_output(char *loopfilename) {
  FILE *fp = fopen(loopfilename,"a");
  if (fp == NULL) {
      perror("fopen failed");
      return;
  }
  for (int i = 0; i < loop_index; i++) {
      fprintf(fp, "%d,%.9f,%d,%d,%d\n", i + 1, (loop_end[i] - loop_start[i]) / (double)CPU_FREQ_HZ, poll_ret[i], poll_num[i] + 1, disturb_num);
      //printf("%d 番目, 回数: %d\n", i + 1, loop_num_array[i]);
  }
}

// ループ回数出力用関数
// loop_num_array に格納されている値を出力(何番目のループか，ループ回数)
void loop_num_output() {
  int max = 0;

  for (int i = 0; i < repeat_cnt; i++) {
      //printf("%d 番目, 回数: %d\n", i + 1, loop_num_array[i]);
      if (loop_num_array[i] > max)
        max = loop_num_array[i];
  }

  int hist[max + 1];
  memset(hist, 0, sizeof(hist));

  for (int i = 0; i < repeat_cnt; i++) {
      hist[loop_num_array[i]]++;
  }

  for (int i = 0; i <= max; i++) {
      if (hist[i] > 0) {
          printf("ループ回数 %d 回 : %d 回発生\n", i, hist[i]);
      }
  }
}

//#include "log.h"
//
//// 送受信計測用
//int global_send_cnt = 0;
//int global_recv_cnt = 0;
//int global_send_err_cnt  = 0;
//int global_recv_timeout_cnt = 0;
//
//// ppoll 回数計測用
//int global_ppoll_cnt = 0;
//
//// ログファイル用
//FILE *log_fp;
//char log_name[100];
//
//void open_logfile(char *fmt, ...) {
//  va_list args;
//  va_start(args, fmt);
//  vsprintf(log_name, fmt, args);
//  va_end(args);
//
//  log_fp = fopen(log_name, "w");
//}
//
//void close_logfile() {
//  fclose(log_fp);
//}
//
//void logfile_printf(char *fmt, ...) {
//  va_list args;
//  va_start(args, fmt);
//  vfprintf(log_fp, fmt, args);
//  va_end(args);
//}
//
