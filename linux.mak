#
# Linux Makefile
#
# usage:
#  make -f linux.mak
#

BIN_DIR=bin
BUILD_DIR=.build
OBJ_DIR=${BUILD_DIR}/obj
SRC_DIR=.


# gl/glut needed for graphics
LDFLAGS+=-lGL -lglut

# expat needed for eschew
LDFLAGS+=-lexpat


TARGET=${BIN_DIR}/colditz

OBJS=low-level.o soundplayer.o videoplayer.o md5.o game.o graphics.o eschew/ConvertUTF.o eschew/eschew.o conf.o main.o

TARGET.objs=$(patsubst %.o,${OBJ_DIR}/%.o,${OBJS})

all: ${TARGET}
	@echo "DONE."

${TARGET}: ${TARGET.objs}
	test -d ${dir $@}|| mkdir -p ${dir $@} && ${LINK.cc} $^ -o $@

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c
	test -d ${dir $@} || mkdir -p ${dir $@} && ${COMPILE.c} $^ -o $@
	
${OBJ_DIR}/%.o: ${SRC_DIR}/%.cpp
	test -d ${dir $@} || mkdir -p ${dir $@} && ${COMPILE.cc} $^ -o $@
	
clean:
	${RM} -f ${TARGET}
	${RM} -rf ${BIN_DIR}
	${RM} -rf ${OBJ_DIR}
	${RM} -rf ${BUILD_DIR}

