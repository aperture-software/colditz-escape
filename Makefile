TARGET = colditz
OBJS = psp/psp-setup.o low-level.o soundplayer.o utilities.o getopt.o main.o

INCDIR = 
;CFLAGS = -O2 -G0 -Wall
CFLAGS = -O3 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
FLAGS =
LIBS += -lglut -lGLU -lGL -lpspgu -lm -lc -lpsprtc -lpspaudiolib -lpspaudio
# -lpspvfpu

BUILD_PRX=1
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Colditz

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
