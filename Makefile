# =====================
# general valiabels
# =====================
CC = gcc

TARGET = test
SRC = ${TARGET}.c

MYLIB = mysoem.c
LIB_DIR = soem/${TARGET}

UTILS = soem/log.c soem/timer.c

ORIGIN_LIB_DIR = soem/origin
ARCHIVE_DIR = archive
ARCHIVE_FILE = ${ARCHIVE_DIR}/lib${TARGET}.a

# =====================
# Directory of search src files
# =====================
VPATH = ${SRC_DIRS}

SRCS = $(wildcard ${LIB_DIR}/*.c)
OBJS = $(patsubst %.c, %.o, $(notdir ${SRCS}))
CFLAGS = -O2
CFLAGS += $(addprefix -I, ./${LIB_DIR})

# プロジェクトの作成
project:
	touch ${SRC}
	cp -r ${ORIGIN_LIB_DIR} soem/${TARGET}


${OBJS}: ${SRCS} 
	${CC} -c ${CFLAGS} $^ 


check: 
	@echo "[INFO] SRCS: ${SRCS}"
	@echo "[INFO] OBJS: ${OBJS}"
	@echo "[INFO] CFLAGS: ${CFLAGS}"


archive: ${OBJS}
	ar rcs ${ARCHIVE_FILE} $^
	rm ${OBJS}


exe: 
	${CC} -o ${TARGET} ${SRC} ${MYLIB} ${ARCHIVE_FILE} ${UTILS} -I./soem
