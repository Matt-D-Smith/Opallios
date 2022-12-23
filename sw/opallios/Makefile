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

bins-y += opallios

all: $(bins-y)

$(bins-y):
	$(CC) -o $@ $^ -lGLESv2 -lEGL -ldrm -lgbm -lpthread -lrt -lm -ldl

opallios: opallios.o ../bridge_lib/bw_bridge.o libraylib.a

clean:
	$(RM) *.o *~ $(bins-y)

-include .*.d
