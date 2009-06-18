TARGET = colditz
OBJS = psp/psp-setup.o low-level.o soundplayer.o videoplayer.o md5.o utilities.o getopt.o main.o
#OBJS = mainer.o

INCDIR = 
;CFLAGS = -O2 -G0 -Wall
CFLAGS = -O3 -Wall -G0 
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

#LIBS += -lpsphprm_driver
# -lpspvfpu

LIBDIR = .
FLAGS =
LIBS += -lglut -lGLU -lGL -lpmp -lpspgum -lpspgu -lpsprtc -lm -lc -lpspaudiolib -lpspaudio -lpspaudiocodec -lpspmpeg 
# -lz   -lpspctrl -lpsppower -lpspaudio 

BUILD_PRX = 1
EXTRA_TARGETS = EBOOT.PBP

PSP_DIR_NAME = Colditz
PSP_EBOOT_SFO = param.sfo
PSP_EBOOT_TITLE = Escape From Colditz PSP v0.9.2
PSP_EBOOT = EBOOT.PBP
PSP_EBOOT_ICON = icon1.png
PSP_EBOOT_ICON1 = NULL
PSP_EBOOT_PIC0 = NULL
PSP_EBOOT_PIC1 = NULL
PSP_EBOOT_SND0 = NULL
PSP_EBOOT_PSAR = NULL
PSP_FW_VERSION = 371

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

#LIBS += -lpsphprm_driver -lpmp