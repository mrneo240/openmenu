include Makefile.inc

# Can be: ui/draw_console.c OR  ui/draw_kos.c OR ui/draw_gamejam.c
OUTPUT := ui/draw_console.c

SRCS := backend/ini.c backend/gd_list.c ui/ui_line_large.c ui/ui_line_desc.c ui/pc/font.c $(OUTPUT) example.c
OBJS = $(subst .c,.o,$(SRCS))

example: $(OBJS)
		@$(ECHO) CC -o $@
		$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
		@$(ECHO) CC -c $< -o $@
		$(CC) $(PRJCFLAGS) $(CFLAGS) -c $< -o $@

ui/draw_console.o: ui/draw_prototypes.h
ui/ui_line_desc.o: ui/draw_prototypes.h

.PHONY: clean

clean:
	@-$(RM) $(OBJS)

#tool.o: tool.cc support.hh
#    g++ $(CPPFLAGS) -c tool.cc
#
#support.o: support.hh support.cc
#    g++ $(CPPFLAGS) -c support.cc