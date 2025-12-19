#!/bin/bash -eu
clang -I/usr/include/bpf -I../scx/build/scheds/c/scx_priority.p/ -O0 -g -Wall -c infinityloop.c -o infinityloop.o
clang -I/usr/include/bpf -I../scx/build/scheds/c/scx_priority.p/ -O0 -g -Wall infinityloop.o -o infinityloop -lbpf

clang -I/usr/include/bpf -I../scx/build/scheds/c/scx_priority.p/ -O0 -g -Wall -c co-task.c -o co-task.o
clang -I/usr/include/bpf -I../scx/build/scheds/c/scx_priority.p/ -O0 -g -Wall co-task.o -o co-task -lbpf

rm infinityloop.o
rm co-task.o
