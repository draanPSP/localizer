TARGET = localizer
OBJS = main.o 

INCDIR = 
CFLAGS = -O2 -G0 -Wall -fno-pic
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

USE_KERNEL_LIBS=1
USE_KERNEL_LIBC=1

LIBDIR = 
LDFLAGS = -mno-crt0 -nostartfiles
LIBS = -lpspsystemctrl_kernel


PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak


