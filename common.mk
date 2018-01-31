# This Makefile is used for all source codes that must build binaries.

DEFAULT_CFLAGS = -O2 -s

ifndef CC
ifeq (,$(shell which gcc 2>&1 >/dev/null))
CC=gcc
else
CC=cc
endif
endif
