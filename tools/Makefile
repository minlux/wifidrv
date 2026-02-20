# Makefile for MBR, VBR, FAT and Root Directory reading tools

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGETS = read_mbr read_vbr read_fat read_rootdir

all: $(TARGETS)

read_mbr: read_mbr.c layout.h
	$(CC) $(CFLAGS) -o read_mbr read_mbr.c

read_vbr: read_vbr.c layout.h
	$(CC) $(CFLAGS) -o read_vbr read_vbr.c

read_fat: read_fat.c layout.h
	$(CC) $(CFLAGS) -o read_fat read_fat.c

read_rootdir: read_rootdir.c layout.h
	$(CC) $(CFLAGS) -o read_rootdir read_rootdir.c

clean:
	rm -f $(TARGETS)

.PHONY: all clean
