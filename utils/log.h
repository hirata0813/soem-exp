#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdarg.h>

// 送受信回数計測用
extern int global_send_cnt;
extern int global_recv_cnt;
extern int global_send_err_cnt;
extern int global_recv_timeout_cnt;

// ppoll 回数計測用
extern int global_ppoll_cnt;

void open_logfile(char *fmt, ...);
void close_logfile();
void logfile_printf(char *fmt, ...);
void logfile_output();

#endif

//#ifndef LOG_H
//#define LOG_H
//
//#include <stdio.h>
//#include <stdint.h>
//#include <unistd.h>
//#include <stdarg.h>
//
//// 送受信回数計測用
//extern int global_send_cnt;
//extern int global_recv_cnt;
//extern int global_send_err_cnt;
//extern int global_recv_timeout_cnt;
//
//// ppoll 回数計測用
//extern int global_ppoll_cnt;
//
//void open_logfile(char *fmt, ...);
//void close_logfile();
//void logfile_printf(char *fmt, ...);
//
//#endif
//
