#ifndef MYSOEM_H
#define MYSOEM_H

#include "soem/timestamp/soem.h"
#include <stdio.h>
#include <string.h>

// Slave の状態を文字列に変換
char *sstate_to_string(char *str, uint16 state);
// Sync Manager のタイプを文字列に変換
char *smtype_to_string(char *str, uint8 state);
// FMMU のタイプを文字列に変換
char *fmmufunc_to_string(char *str, uint8 func);


int print_slaveinfo(ec_slavet *slave); 

#endif