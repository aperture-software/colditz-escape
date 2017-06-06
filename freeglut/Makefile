TARGET   = libfreeglut.a
CC       = gcc
OBJS     = fg_callbacks.o fg_cursor.o fg_display.o fg_ext.o fg_font.o fg_font_data.o fg_gamemode.o
OBJS    += fg_geometry.o fg_gl2.o fg_init.o fg_input_devices.o fg_joystick.o fg_main.o fg_menu.o
OBJS    += fg_misc.o fg_overlay.o fg_spaceball.o fg_state.o fg_stroke_mono_roman.o fg_stroke_roman.o
OBJS    += fg_structure.o fg_teapot.o fg_videoresize.o fg_window.o util/xparsegeometry_repl.o
OBJS    += x11/fg_cursor_x11.o x11/fg_display_x11_glx.o x11/fg_ext_x11.o x11/fg_gamemode_x11.o
OBJS    += x11/fg_glutfont_definitions_x11.o x11/fg_init_x11.o x11/fg_input_devices_x11.o
OBJS    += x11/fg_joystick_x11.o x11/fg_main_x11.o x11/fg_menu_x11.o x11/fg_spaceball_x11.o
OBJS    += x11/fg_state_x11.o x11/fg_state_x11_glx.o x11/fg_structure_x11.o x11/fg_window_x11.o
OBJS    += x11/fg_window_x11_glx.o x11/fg_xinput_x11.o
CFLAGS  += -DENHANCED_XBOX_360_CONTROLLER_SUPPORT -I. -I.. -w -O3

all: $(TARGET)

.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

clean:
	$(RM) $(OBJS) $(TARGET)
