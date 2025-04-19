# Makefile
#
# The T# Programming Language
# Copyright Dylan Armstrong Research and Development 2025

CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
LDFLAGS = -mconsole

SRC_DIR = src

TARGET = tsharp.exe

SRCS = $(wildcard $(SRC_DIR)/*.c) Main.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

clean:
	@if exist $(TARGET) del $(TARGET)
	@if exist Main.o del Main.o
	@for %%f in (src\*.o) do @if exist %%f del %%f

.PHONY: all clean