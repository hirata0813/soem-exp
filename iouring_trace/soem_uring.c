#include "soem_uring.h"
#include <stdio.h>

IOuringContext ioctx;

int iouring_init() {
  return io_uring_queue_init(IOURING_QDEPTH, &(ioctx.ring), 0);
}

void iouring_deinit() {
  io_uring_queue_exit(&(ioctx.ring));
}

int iouring_request_send_recv(int sock, void *txbuf, size_t txlen, void *rxbuf, size_t rxlen, int flag) {
  int submission = 0;
  ioctx.sq = io_uring_get_sqe(&(ioctx.ring));
  if (ioctx.sq == NULL) {
    printf("[ERROR] fail to iouring submissonQ\n");
    return -1;
  }
  // send syscall 要求の登録
  io_uring_prep_send(ioctx.sq, sock, txbuf, txlen, flag);
  io_uring_sqe_set_data(ioctx.sq, (void *)REQ_ID_SEND);
  ioctx.sq->flags |= IOSQE_IO_LINK;

  ioctx.sq = io_uring_get_sqe(&(ioctx.ring));
  if (ioctx.sq == NULL) {
    printf("[ERROR] fail to iouring submissonQ\n");
    return -1;
  }
  io_uring_prep_recv(ioctx.sq, sock, rxbuf, rxlen, flag);
  io_uring_sqe_set_data(ioctx.sq, (void *)REQ_ID_RECV);

  submission = io_uring_submit(&(ioctx.ring));
  if (submission <= 0) {
    printf("[ERROR] fail to iouring send-recv request submission\n");
    return -1;
  }

  // printf("[INFO] subit send request\n");

  return submission;
}

int iouring_request_send(int sock, void *buf, size_t len, int flag) {
  // printf("[CALL] iouring_request_send\n");
  // submission queue を確保
  int submission = 0;
  ioctx.sq = io_uring_get_sqe(&(ioctx.ring));
  if (ioctx.sq == NULL) {
    printf("[ERROR] fail to iouring submissonQ\n");
    return -1;
  }
  // send syscall 要求の登録
  io_uring_prep_send(ioctx.sq, sock, buf, len, flag);
  io_uring_sqe_set_data(ioctx.sq, (void *)REQ_ID_SEND);
  submission = io_uring_submit(&(ioctx.ring));
  if (submission <= 0) {
    printf("[ERROR] fail to iouring send request submission\n");
    return -1;
  }

  // printf("[INFO] subit send request\n");

  return submission;
}

int iouring_wait_send_completion() {
  // printf("[CALL] iouring_wait_send_completion\n");
  int completion = io_uring_wait_cqe(&(ioctx.ring), &(ioctx.cq));
  if (completion < 0) {
    perror("[ERROR] fail to iouring wait send\n");
    return -1;
  }

  if (ioctx.cq->user_data != REQ_ID_SEND) {
    return -1;
  }
  // completion queue の後始末
  io_uring_cqe_seen(&(ioctx.ring), ioctx.cq);
  // printf("[INFO] complete send request\n");
  // send の返り値
  return ioctx.cq->res;
}

int iouring_request_recv(int sock, void *buf, size_t len, int flag) {
  // printf("[CALL] iouring_request_recv\n");
  // submission queue を確保
  int submission = 0;
  ioctx.sq = io_uring_get_sqe(&(ioctx.ring));
  if (ioctx.sq == NULL) {
    printf("[ERROR] fail to iouring sunmissionQ\n");
    return -1;
  }
  // send syscall 要求の登録
  io_uring_prep_recv(ioctx.sq, sock, buf, len, flag);
  io_uring_sqe_set_data(ioctx.sq, (void *)REQ_ID_RECV);
  submission = io_uring_submit(&(ioctx.ring));
  if (submission <= 0) {
    printf("[ERROR] fail to iouring recv request submission\n");
    return -1;
  }

  // printf("[INFO] subit recv request\n");

  return submission;
}

// ノンブロッキングで recv の完了をチェック
int iouring_wait_recv_completion() {
  // printf("[CALL] iouring_wait_recv_completion\n");
  // int ret = io_uring_peek_cqe(&(ioctx.ring), &(ioctx.cq));
  int ret = io_uring_wait_cqe(&(ioctx.ring), &(ioctx.cq));

  // 未完了
  if (ret < 0) {
    printf("[ERROR] fail to iouring wait recv\n");
    return -1;
  }

  if (ioctx.cq->user_data != REQ_ID_RECV) {
    return -1;
  }
  io_uring_cqe_seen(&(ioctx.ring), ioctx.cq);
  // printf("[INFO] complete recv request\n");
  
  return ioctx.cq->res;
}

// ノンブロッキングで recv の完了をチェック
int iouring_check_recv_completion() {
  // int ret = io_uring_peek_cqe(&(ioctx.ring), &(ioctx.cq));
  int ret = io_uring_wait_cqe(&(ioctx.ring), &(ioctx.cq));

  // 未完了
  if (ret < 0) {
    printf("[ERROR] fail to iouring wait recv\n");
    return -1;
  }

  // if (ioctx.cq->user_data != REQ_ID_RECV) {
  //   return 0;
  // }
  // printf("[INFO] complete recv request\n");
  
  return ret;
}

int iouring_finish_recv() {
  // completion queue の後始末
  io_uring_cqe_seen(&(ioctx.ring), ioctx.cq);
  // recv の返り値
  return ioctx.cq->res;
}