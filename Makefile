TARGET = colditz
OBJS = psp/psp-setup.o low-level.o soundplayer.o videoplayer.o md5.o game.o graphics.o eschew/ConvertUTF.o eschew/eschew.o conf.o main.o

INCDIR = 
CFLAGS = -O3 -Wall -G0 -Xlinker -S -Xlinker -x
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR = ./libs
FLAGS =
LIBS += -lglut -lGLU -lGL -lpmp -lexpat -lpspgum -lpspgu -lpsprtc -lm -lc -lpspaudiolib -lpspaudio -lpspaudiocodec -lpspmpeg -lpsppower 
# IMPORTANT NOTE: If you try to use a PSPGL that relies on VFPU (i.e. need to add -lpspvfpu above), the game will be SUPER-SLOW!!!
# Don't know what the heck has changed in the VFPU based versions of PSPGL, but it sure didn't improve 2D performance
# To get *proper* GL speed, you MUST use the 2005.12.10 version of the PSPGL, which can be obtained from:
# http://hg.goop.org/pspgl/jsgf?cmd=archive;node=working;type=zip or the pspgl archive one included with this source in extras/

BUILD_PRX = 1
EXTRA_TARGETS = EBOOT.PBP

PSP_DIR_NAME = Colditz
PSP_EBOOT_SFO = param.sfo
PSP_EBOOT_TITLE = Colditz Escape! PSP v0.9.2
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

