include Makefile.inc

SRCS = backend/ini.c backend/gd_list.c ui/ui_line_large.c ui/ui_line_desc.c example.c
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