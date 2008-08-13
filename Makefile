TARGET = colditz
OBJS = main.o utilities.o getopt.o psp/vram.o 

INCDIR = 
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS= -lpspgu

BUILD_PRX=1
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Colditz

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
