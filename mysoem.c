#include "mysoem.h"

char *sstate_to_string(char *str, uint16 state) {
  switch (state) {
    case EC_STATE_NONE:
      sprintf(str, "None"); break;
    case EC_STATE_INIT:
      sprintf(str, "Init"); break;
    case EC_STATE_PRE_OP:
      sprintf(str, "Pre Operational"); break;
    case EC_STATE_SAFE_OP:
      sprintf(str, "Safe Operational"); break;
    case EC_STATE_OPERATIONAL:
      sprintf(str, "Operational"); break;
    case EC_STATE_ERROR:
      sprintf(str, "Error"); break;
  }

  return str;
}

char *smtype_to_string(char *str, uint8 state) {
  switch (state) {
    case 0:
      sprintf(str, "Unused"); break;
    case 1:
      sprintf(str, "Mail box (write)"); break;
    case 2:
      sprintf(str, "Main box (read)"); break;
    case 3:
      sprintf(str, "Output"); break;
    case 4:
      sprintf(str, "Input"); break;
  }

  return str; 
}

char *fmmufunc_to_string(char *str, uint8 func) {
  switch (func) {
    case 0:
      sprintf(str, "Unused"); break;
    case 1:
      sprintf(str, "Output"); break;
    case 2:
      sprintf(str, "Input"); break;
    case 3:
      sprintf(str, "SM status"); break;
  }

  return str; 
}


int print_slaveinfo(ec_slavet *slave) {
  char str[100];
  memset(str, 0, 100);

  printf("Device Name: %s\nState: %s", slave->name, sstate_to_string(str, slave->state));
  printf("Output bits: %d Input bits: %d\n", slave->Obits, slave->Ibits);
  
  for (int i = 0; i < EC_MAXSM; i++) {
    if (slave->SM[i].StartAddr > 0)
      printf("[SM%1d]\n Start Addr:%4.4x\n Length:%4d\n Flag:%8.8x\n Type:%s\n", i, slave->SM[i].StartAddr, slave->SM[i].SMlength,
              slave->SM[i].SMflags, smtype_to_string(str, slave->SMtype[i]));
  }

  for (int i = 0; i < slave->FMMUunused; i++) {
    printf("[FMMU%1d]\n offset:%8.8x\n len:%4d\n Lsb:%d Leb:%d\n Ps:%4.4x Psb:%d\n Type:%s\n Act:%2.2x\n", 
          i, slave->FMMU[i].LogStart, slave->FMMU[i].LogLength, slave->FMMU[i].LogStartbit,
          slave->FMMU[i].LogEndbit, slave->FMMU[i].PhysStart, slave->FMMU[i].PhysStartBit,
          fmmufunc_to_string(str, slave->FMMU[i].FMMUtype), slave->FMMU[i].FMMUactive);
  }

  
} 