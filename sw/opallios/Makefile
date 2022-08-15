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
bins-y += opalliosgpu
bins-y += triangle

all: $(bins-y)

$(bins-y):
	$(CC) -o $@ $^ -lGLESv2 -lEGL -ldrm -lgbm -lpthread -lrt -lm -ldl -lglfw

opallios: opallios.o bw_bridge.o libraylib.a
opalliosgpu: opalliosgpu.o bw_bridge.o libraylib.a
triangle: triangle.o bw_bridge.o

clean:
	$(RM) *.o *~ $(bins-y)

-include .*.d
