#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <time.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <poll_count>\n", argv[0]);
        return 1;
    }

    int max_poll = atoi(argv[1]);
    if (max_poll <= 0) {
        fprintf(stderr, "poll_count must be > 0\n");
        return 1;
    }

    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;

    struct timespec timeout;
    timeout.tv_sec  = 0;
    timeout.tv_nsec = 100 * 1000 * 1000;  // 100 ms

    sigset_t sigmask;
    sigemptyset(&sigmask);  // シグナルマスクなし

    char buf[256];

    for (int i = 0; i < max_poll; i++) {
        int ret = ppoll(&pfd, 1, &timeout, &sigmask);

        if (ret < 0) {
            perror("ppoll");
            break;
        }

        if (ret == 0) {
            /* タイムアウト */
            printf("[poll %d] timeout\n", i + 1);
            continue;
        }

        if (pfd.revents & POLLIN) {
            ssize_t n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                printf("[poll %d] read: %s", i + 1, buf);
            }
        }
    }

    return 0;
}
