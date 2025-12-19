CC = clang
CLANG_INCLUDES = -I/usr/include/bpf -I../../scx/build/scheds/c/scx_priority.p/ -I../utils
CFLAGS = $(CLANG_INCLUDES) -O0 -g -Wall

SRC = pt-prio-whole.c
TARGET = ${SRC:.c=}

SOEM_SRCS = $(wildcard soem/*.c)
SOEM_OBJS = $(patsubst soem/%.c, %.o, $(SOEM_SRCS))

UTIL_SRCS = $(wildcard ../utils/*.c)
UTIL_OBJS = $(patsubst ../utils/%.c, %.o, $(UTIL_SRCS))

ALL_OBJS = $(SOEM_OBJS) $(UTIL_OBJS)

ARCHIVE_FILE = libsoem.a

%.o: soem/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: ../utils/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

archive: ${SOEM_OBJS}
	ar rcs ${ARCHIVE_FILE} $^
	rm ${SOEM_OBJS}

exe: archive ${SRC} ${UTIL_OBJS}
	$(CC) -o ${TARGET} ${SRC} ${ARCHIVE_FILE} ${UTIL_OBJS} $(CFLAGS) -I./soem -L. -lbpf
	rm log.o timer.o

.PHONY: archive exe clean
clean:
	rm -f ${ALL_OBJS} ${ARCHIVE_FILE} ${TARGET}
