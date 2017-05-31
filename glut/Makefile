TARGET   = libglut.a
CC       = gcc
OBJS     = glut_8x13.o glut_9x15.o glut_bitmap.o glut_bwidth.o glut_cindex.o glut_cmap.o glut_cursor.o glut_dials.o
OBJS    += glut_dstr.o glut_event.o glut_ext.o glut_fullscrn.o glut_gamemode.o glut_get.o glut_glxext.o glut_hel10.o
OBJS    += glut_hel12.o glut_hel18.o glut_init.o glut_input.o glut_joy.o glut_key.o glut_keyctrl.o glut_keyup.o
OBJS    += glut_mesa.o glut_modifier.o glut_mroman.o glut_overlay.o glut_roman.o glut_shapes.o glut_space.o glut_stroke.o
OBJS    += glut_swap.o glut_swidth.o glut_tablet.o glut_teapot.o glut_tr10.o glut_tr24.o glut_util.o glut_vidresize.o
OBJS    += glut_warp.o glut_win.o glut_winmisc.o layerutil.o
CFLAGS  += -I. -w -O3

all: $(TARGET)

.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

clean:
	$(RM) $(OBJS) $(TARGET)
