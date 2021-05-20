include Makefile.inc

# Can be: ui/draw_console.c OR  ui/draw_kos.c OR ui/draw_gamejam.c
OUTPUT := ui/draw_console.c

SRCS := backend/ini.c backend/gd_list.c ui/ui_line_large.c ui/ui_line_desc.c $(OUTPUT) example.c
OBJS = $(subst .c,.o,$(SRCS))

example: $(OBJS)
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
		$(CC) $(PRJCFLAGS) $(CFLAGS) -c $< -o $@

#tool.o: tool.cc support.hh
#    g++ $(CPPFLAGS) -c tool.cc
#
#support.o: support.hh support.cc
#    g++ $(CPPFLAGS) -c support.cc