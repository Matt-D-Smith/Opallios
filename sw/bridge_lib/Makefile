ARCH = arm
CROSS ?= arm-linux-gnueabihf-
CC = $(CROSS)gcc
PWD := $(shell pwd)

CFLAGS = \
	-std=c11 \
	-g \
	-O3 \
	-W \
	-Wall \
	-Wextra \
	-Wp,-MMD,$(dir $@).$(notdir $@).d \
	-Wp,-MT,$@ \

bins-y += sdram
bins-y += memmap
bins-y += opallios

all: $(bins-y)

$(bins-y):
	$(CC) -o $@ $^ -lGLESv2 -lEGL -ldrm -lgbm -lpthread -lrt -lm -ldl

sdram: sdram.o bw_bridge.o
memmap: memmap.o bw_bridge.o
opallios: opallios.o bw_bridge.o libraylib.a

clean:
	$(RM) *.o *~ $(bins-y)

-include .*.d
