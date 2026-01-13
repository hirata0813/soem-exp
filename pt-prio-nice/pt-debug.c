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


// ========== 🆕 カスタム PDO 設定関数 ==========
static int
custom_pdo_mapping(uint16 slave)
{
   int retval = 0;
   // この関数は ecx_config_init から呼ばれる
   // 戻り値: 0=自動マッピング使用, >0=手動設定完了
   
   printf("Using custom PDO mapping for slave %d\n", slave);
   
   // 何もしない（EEPROM の SM 設定をそのまま使う）
   // slaveinfo で見えている SM2/SM3 の設定がそのまま使われる
   
   return 1;  // 手動設定完了を示す
}
// =============================================

static boolean
fieldbus_start(Fieldbus *fieldbus)
{
   ecx_contextt *context;
   ec_groupt *grp;
   ec_slavet *slave;
   int i;
   uint16 state_check, al_control, al_status;

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

   slave = &context->slavelist[1];
   
   // ========== 手動 PDO 設定 ==========
   slave->SM[2].SMlength = 2;
   slave->SMtype[2] = 3;
   slave->SM[3].SMlength = 6;
   slave->SMtype[3] = 4;
   slave->Obits = 16;
   slave->Ibits = 48;
   slave->Obytes = 2;
   slave->Ibytes = 6;
   slave->FMMU0func = 1;
   slave->FMMU1func = 2;
   
   printf("\n=== Manual PDO Configuration ===\n");
   printf("Set Output:  %d bits (%d bytes)\n", slave->Obits, slave->Obytes);
   printf("Set Input: %d bits (%d bytes)\n", slave->Ibits, slave->Ibytes);
   printf("================================\n\n");
   
   printf("Sequential mapping of I/O...  ");
   ecx_config_map_group(context, fieldbus->map, fieldbus->group);
   printf("mapped %dO+%dI bytes\n", grp->Obytes, grp->Ibytes);

   printf("Configuring distributed clock... ");
   ecx_configdc(context);
   printf("done\n");

   // ========== 🆕 レジスタ直接読み取りで診断 ==========
   printf("\n=== Register Diagnostics (INIT state) ===\n");
   
   // AL Control レジスタ (0x0120)
   ecx_FPRD(&context->port, slave->configadr, ECT_REG_ALCTL, sizeof(al_control), 
            &al_control, EC_TIMEOUTRET);
   printf("AL Control:   0x%04X\n", al_control);
   
   // AL Status レジスタ (0x0130)
   ecx_FPRD(&context->port, slave->configadr, ECT_REG_ALSTAT, sizeof(al_status), 
            &al_status, EC_TIMEOUTRET);
   printf("AL Status:  0x%04X\n", al_status);
   
   // DL Status レジスタ (0x0110) - リンク状態
   uint16 dl_status;
   ecx_FPRD(&context->port, slave->configadr, ECT_REG_DLSTAT, sizeof(dl_status), 
            &dl_status, EC_TIMEOUTRET);
   printf("DL Status:  0x%04X\n", dl_status);
   
   printf("=========================================\n\n");
   // =================================================

   // ========== 🆕 PRE_OP 遷移を詳細にトレース ==========
   printf("Manually transitioning to PRE_OP.. .\n");
   
   // ステップ1: AL Control に PRE_OP を書き込む
   printf("  [1] Writing 0x0002 to AL Control (0x0120)...  ");
   al_control = EC_STATE_PRE_OP;  // 0x0002
   ecx_FPWR(&context->port, slave->configadr, ECT_REG_ALCTL, sizeof(al_control), 
            &al_control, EC_TIMEOUTRET);
   printf("done\n");
   
   // ステップ2: 少し待つ
   osal_usleep(10000);  // 10ms
   
   // ステップ3: AL Control が書き込まれたか確認
   printf("  [2] Reading back AL Control...  ");
   ecx_FPRD(&context->port, slave->configadr, ECT_REG_ALCTL, sizeof(al_control), 
            &al_control, EC_TIMEOUTRET);
   printf("0x%04X\n", al_control);
   
   // ステップ4: AL Status をポーリング
   printf("  [3] Polling AL Status for state change:\n");
   for (i = 0; i < 50; ++i)  // 最大 5 秒
   {
      osal_usleep(100000);  // 100ms
      
      ecx_FPRD(&context->port, slave->configadr, ECT_REG_ALSTAT, sizeof(al_status), 
               &al_status, EC_TIMEOUTRET);
      
      uint16 current_state = al_status & 0x000F;
      uint16 error_flag = al_status & 0x0010;
      uint16 al_status_code = (al_status >> 8) & 0x00FF;
      
      printf("      [%2d] AL Status=0x%04X (State=0x%X, Error=%d, Code=0x%02X)\n",
             i, al_status, current_state, error_flag ?  1 : 0, al_status_code);
      
      if (current_state == EC_STATE_PRE_OP)
      {
         printf("  [SUCCESS] Reached PRE_OP!\n");
         break;
      }
      
      if (error_flag)
      {
         printf("  [ERROR] AL Status Error flag set!\n");
         
         // AL Status Code レジスタ (0x0134) を読む
         uint16 detailed_code;
         ecx_FPRD(&context->port, slave->configadr, ECT_REG_ALSTATCODE, 
                  sizeof(detailed_code), &detailed_code, EC_TIMEOUTRET);
         printf("  AL Status Code (0x0134): 0x%04X (%s)\n",
                detailed_code, ec_ALstatuscode2string(detailed_code));
         break;
      }
      
      // 10回ごとに詳細表示
      if (i % 10 == 9)
      {
         // Mailbox の状態も確認
         uint8 sm0_status, sm1_status;
         ecx_FPRD(&context->port, slave->configadr, ECT_REG_SM0STAT, 1, 
                  &sm0_status, EC_TIMEOUTRET);
         ecx_FPRD(&context->port, slave->configadr, ECT_REG_SM1STAT, 1, 
                  &sm1_status, EC_TIMEOUTRET);
         printf("      SM0 Status=0x%02X, SM1 Status=0x%02X\n", 
                sm0_status, sm1_status);
      }
   }
   
   // 最終確認
   ecx_readstate(context);
   printf("\n  Final slave state: 0x%04X\n", slave->state);
   printf("  Final AL status code: 0x%04X\n", slave->ALstatuscode);
   
   if (slave->state != EC_STATE_PRE_OP)
   {
      printf("\n[FAILED] Could not transition to PRE_OP\n");
      printf("This indicates a firmware issue in PIC24:\n");
      printf("  - AL Control register may not be monitored\n");
      printf("  - State machine may not be implemented\n");
      printf("  - Mailbox (SM0/SM1) may need initialization\n");
      return FALSE;
   }
   // ==================================================

   printf("\nWaiting for SAFE_OP...\n");
   slave->state = EC_STATE_SAFE_OP;
   ecx_writestate(context, 0);
   ecx_statecheck(context, 0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);
   ecx_readstate(context);
   
   if (slave->state != EC_STATE_SAFE_OP)
   {
      printf("FAILED to reach SAFE_OP\n");
      printf("Current state: 0x%04X\n", slave->state);
      printf("AL status: 0x%04X (%s)\n", 
             slave->ALstatuscode, ec_ALstatuscode2string(slave->ALstatuscode));
      return FALSE;
   }

   printf("Send a roundtrip...  ");
   fieldbus_roundtrip(fieldbus);
   printf("done\n");

   printf("Setting OPERATIONAL.. .\n");
   slave->state = EC_STATE_OPERATIONAL;
   ecx_writestate(context, 0);
   
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

   printf(" failed\n");
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
uint32_t io_cnt = 0;

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
  //sleep(30);
  sprintf(fname, "simple-soem-task-result.csv");

  // valiables for test
  char nic[10] = "eno1";
  repeat_cnt = atoi(argv[1]);
  disturb_num = atoi(argv[2]);
  io_start = (double*)malloc(sizeof(unsigned long long) * repeat_cnt);
  io_end = (double*)malloc(sizeof(unsigned long long) * repeat_cnt);
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

    // 以下の for ループ内で I/O 処理を担当
    for (i = 0; i < repeat_cnt; ++i)
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

      osal_usleep(interval_usec);
    }

  }

  // 以下で，ログファイルに計測結果を吐き出す
  // まず，io_cntとrepeat_cntが等しいかチェック
  if (io_cnt != repeat_cnt){
    perror("io_cnt != repeat_cnt");
    return 1;
  }
  logfile_output(fname);

  printf("\nio_cnt == repeat_cnt!\n");
  printf("\n[INFO] send cnt: %d\n", global_send_cnt);
  printf("\n[INFO] recv cnt: %d\n", global_recv_cnt);
  printf("\n[INFO] send_err cnt:  %d\n", global_send_err_cnt);
  printf("\n[INFO] recv_timout cnt:  %d\n", global_recv_timeout_cnt);

  fieldbus_stop(&fieldbus);
  //close_logfile();
  free(io_start);
  free(io_end);

  return 0;
}
