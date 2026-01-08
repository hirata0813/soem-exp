#include "../utils/utils.h"
#include "./soem/soem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

typedef struct
{
   ecx_contextt context;
   char *iface;
   uint8 group;
   int roundtrip_time;
   uint8 map[4096];
} Fieldbus;

static void
fieldbus_initialize(Fieldbus *fieldbus, char *iface)
{
   /* Let's start by 0-filling `fieldbus` to avoid surprises */
   memset(fieldbus, 0, sizeof(*fieldbus));

   fieldbus->iface = iface;
   fieldbus->group = 0;
   fieldbus->roundtrip_time = 0;
}

static int
fieldbus_roundtrip(Fieldbus *fieldbus)
{
   ecx_contextt *context;
   ec_timet start, end, diff;
   int wkc;

   context = &fieldbus->context;

   start = osal_current_time();
   ecx_send_processdata(context);
   wkc = ecx_receive_processdata(context, EC_TIMEOUTRET);
   end = osal_current_time();
   osal_time_diff(&start, &end, &diff);
   fieldbus->roundtrip_time = (int)(diff.tv_sec * 1000000 + diff.tv_nsec / 1000);

   return wkc;
}

static boolean
fieldbus_start(Fieldbus *fieldbus)
{
   ecx_contextt *context;
   ec_groupt *grp;
   ec_slavet *slave;
   int i;

   context = &fieldbus->context;
   grp = context->grouplist + fieldbus->group;

   printf("Initializing SOEM on '%s'... ", fieldbus->iface);
   if (!ecx_init(context, fieldbus->iface))
   {
      printf("no socket connection\n");
      return FALSE;
   }
   printf("done\n");

   printf("Finding autoconfig slaves... ");
   if (ecx_config_init(context) <= 0)
   {
      printf("no slaves found\n");
      return FALSE;
   }
   printf("%d slaves found\n", context->slavecount);

   printf("Sequential mapping of I/O... ");
   ecx_config_map_group(context, fieldbus->map, fieldbus->group);
   printf("mapped %dO+%dI bytes from %d segments",
          grp->Obytes, grp->Ibytes, grp->nsegments);
   if (grp->nsegments > 1)
   {
      /* Show how slaves are distributed */
      for (i = 0; i < grp->nsegments; ++i)
      {
         printf("%s%d", i == 0 ? " (" : "+", grp->IOsegment[i]);
      }
      printf(" slaves)");
   }
   printf("\n");

   printf("Configuring distributed clock... ");
   ecx_configdc(context);
   printf("done\n");

   printf("Waiting for all slaves in safe operational... ");
   ecx_statecheck(context, 0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);
   printf("done\n");

   printf("Send a roundtrip to make outputs in slaves happy... ");
   fieldbus_roundtrip(fieldbus);
   printf("done\n");

   printf("Setting operational state.. \n");
   /* Act on slave 0 (a virtual slave used for broadcasting) */
   slave = context->slavelist;
   slave->state = EC_STATE_OPERATIONAL;
   ecx_writestate(context, 0);
   /* Poll the result ten times before giving up */
   for (i = 0; i < 10; ++i)
   {
      printf(".");
      fieldbus_roundtrip(fieldbus);
      ecx_statecheck(context, 0, EC_STATE_OPERATIONAL, EC_TIMEOUTSTATE / 10);
      if (slave->state == EC_STATE_OPERATIONAL)
      {
         printf(" all slaves are now operational\n");
         return TRUE;
      }
   }

   printf(" failed,");
   ecx_readstate(context);
   for (i = 1; i <= context->slavecount; ++i)
   {
      slave = context->slavelist + i;
      if (slave->state != EC_STATE_OPERATIONAL)
      {
         printf(" slave %d is 0x%04X (AL-status=0x%04X %s)",
                i, slave->state, slave->ALstatuscode,
                ec_ALstatuscode2string(slave->ALstatuscode));
      }
   }
   printf("\n");

   return FALSE;
}

static void
fieldbus_stop(Fieldbus *fieldbus)
{
   ecx_contextt *context;
   ec_slavet *slave;

   context = &fieldbus->context;
   /* Act on slave 0 (a virtual slave used for broadcasting) */
   slave = context->slavelist;

   printf("Requesting init state on all slaves... ");
   slave->state = EC_STATE_INIT;
   ecx_writestate(context, 0);
   printf("done\n");

   printf("Close socket... ");
   ecx_close(context);
   printf("done\n");
}

uint32_t repeat_cnt;
uint32_t disturb_num;
unsigned long long *io_start;
unsigned long long *io_end;
int *loop_num_array;
uint32_t io_cnt = 0;
uint32_t loop_index = 0;
unsigned long long *loop_start;
unsigned long long *loop_end;
int *poll_num;
int *poll_ret;
unsigned long long soem_start;
unsigned long long soem_end;
extern const unsigned long long CPU_FREQ_HZ;
int soem_init_flag = 0;

int main(int argc, char *argv[])
{
  if (argc < 3) {
    printf("[ERROR] args invalid!\n");
    return 1;
  }

  Fieldbus fieldbus;
  ecx_contextt *context;
  ec_groupt *grp;
  uint32 n;
  int wkc, expected_wkc;
  char fname[128];
  char loopfilename[128];

  // valiables for test
  char nic[10] = "eno1";
  repeat_cnt = atoi(argv[1]);
  int repeat_cnt_copy = repeat_cnt;
  disturb_num = atoi(argv[2]);
  io_start = (double*)malloc(sizeof(unsigned long long) * repeat_cnt);
  io_end = (double*)malloc(sizeof(unsigned long long) * repeat_cnt);
  //loop_num_array = (int*)malloc(sizeof(int) * repeat_cnt);
  //loop_start = (double*)malloc(sizeof(unsigned long long) * repeat_cnt * 5);
  //loop_end = (double*)malloc(sizeof(unsigned long long) * repeat_cnt * 5);
  //poll_num = (int*)malloc(sizeof(int) * repeat_cnt * 5);
  //poll_ret = (int*)malloc(sizeof(int) * repeat_cnt * 5);
  sprintf(fname, "simple-soem-task-result.csv");

  if (!io_start || !io_end) {
      perror("malloc");
      return 1;
  }

  int interval_usec = 20;

  // init logfile
  //open_logfile("log/pt/clock_%d_%s_soem.log", repeat_cnt, id_str);

  fieldbus_initialize(&fieldbus, nic);
  if (fieldbus_start(&fieldbus))
  {
    int i, min_time, max_time;
    min_time = max_time = 0;

    printf("\n[INFO] send cnt: %d\n", global_send_cnt);
    printf("\n[INFO] recv cnt: %d\n", global_recv_cnt);

    context = &(fieldbus.context);
    grp = context->grouplist + fieldbus.group;

    printf("[INFO] Interval: %d (us)\n", interval_usec);
    printf("----- [ Round trip start ] -----\n");
    loop_index = 0;
    io_cnt = 0;
    memset(io_start, 0, sizeof(*io_start));
    memset(io_end, 0, sizeof(*io_end));
    //memset(loop_num_array, 0, sizeof(*loop_num_array));
    //memset(loop_start, 0, sizeof(*loop_start));
    //memset(loop_end, 0, sizeof(*loop_end));
    //memset(poll_num, 0, sizeof(*poll_num));
    //memset(poll_ret, 0, sizeof(*poll_ret));

    // 以下の for ループ内で I/O 処理を担当
    soem_init_flag = 1;
    soem_start = __rdtsc();
    for (i = 0; i < repeat_cnt_copy; ++i)
    {
      ecx_send_processdata(context);
      wkc = ecx_receive_processdata(context, EC_TIMEOUTRET);

      //uint64_t diff_clock_send  = clocks[send_end] - clocks[send_start];
      //uint64_t diff_clock_poll  = clocks[poll_end] - clocks[poll_start];
      //uint64_t diff_clock_recv  = clocks[recv_end] - clocks[recv_start];
      //uint64_t diff_clock_total = clocks[recv_end] - clocks[send_start];

      //logfile_printf("%lld,%lld,%lld,%lld\n", diff_clock_send, diff_clock_poll, diff_clock_recv, diff_clock_total);

      expected_wkc = grp->outputsWKC * 2 + grp->inputsWKC;
      if (wkc == EC_NOFRAME)
      {
          //printf("Round %d: No frame\n", i);
          break;
      }


      // カウンタを1増やす
      io_cnt++;
      //sleep(1);

      // ===========CPU処理区間======================================
      const unsigned long long threshold = 4145055000UL; // 非競合時は，これで大体1分
      unsigned long long cpu_start;
      unsigned long long cpu_end;
      int j = 0;
      cpu_start = __rdtsc();
      if (soem_init_flag){
         while(j <= threshold) {
             j++;
         }
      }
      cpu_end = __rdtsc();
      //printf("CPU 処理=%.9f\n", (cpu_end - cpu_start) / (double)CPU_FREQ_HZ);
      //printf("Init Flag=%d\n", soem_init_flag);
      // ===========CPU処理区間======================================

      osal_usleep(interval_usec);
    }
    soem_end = __rdtsc();

  }

  // 以下で，ログファイルに計測結果を吐き出す
  // まず，io_cntとrepeat_cntが等しいかチェック
  if (io_cnt != repeat_cnt){
    perror("io_cnt != repeat_cnt");
    return 1;
  }
  logfile_output(fname);

  double soem_loop_elapsed = (soem_end - soem_start) / (double)CPU_FREQ_HZ;
  printf("\nio_cnt == repeat_cnt!\n");
  printf("\n[INFO] send cnt: %d\n", global_send_cnt);
  printf("\n[INFO] recv cnt: %d\n", global_recv_cnt);
  printf("\n[INFO] send_err cnt:  %d\n", global_send_err_cnt);
  printf("\n[INFO] recv_timout cnt:  %d\n", global_recv_timeout_cnt);
  //loop_info_output(loopfilename);
  //loop_num_output();

  printf("soem-loop: %.9f\n", soem_loop_elapsed);

  fieldbus_stop(&fieldbus);
  //close_logfile();
  free(io_start);
  free(io_end);
  //free(loop_num_array);
  //free(loop_start);
  //free(loop_end);
  //free(poll_num);
  //free(poll_ret);

  return 0;
}
