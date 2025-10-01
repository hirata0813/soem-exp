#ifndef SOEM_URING_H
#define SOEM_URING_H

#include "liburing.h"

#define IOURING_QDEPTH 32
#define REQ_ID_SEND 0
#define REQ_ID_RECV 1

typedef struct {
  struct io_uring ring;
  struct io_uring_sqe *sq; // submission queue
  struct io_uring_cqe *cq; // completion queue
} IOuringContext;

extern IOuringContext ioctx;

int iouring_init();
void iouring_deinit();
int iouring_request_send_recv(int sock, void *txbuf, size_t txlen, void *rxbuf, size_t rxlen, int flag);
int iouring_request_send(int sock, void *buf, size_t len, int flag);
int iouring_wait_send_completion();
int iouring_request_recv(int sock, void *buf, size_t len, int flag);
int iouring_check_recv_completion();
int iouring_wait_recv_completion();
int iouring_finish_recv();


#endif