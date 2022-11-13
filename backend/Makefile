include ../Makefile.inc

CFLAGS	= $(PRJCFLAGS) -I..
OBJLIBS	= ../libbackend.a
OBJS	= gd_list.o ini.o

all : $(OBJLIBS)

../libbackend.a : $(OBJS)
	$(ECHO) $(AR) $(ARFLAGS) rs $@ $?
	$(AR) $(ARFLAGS) rs $@ $?

clean :
	$(ECHO) cleaning up backend
	-$(RM) -f $(OBJS)

force_look :
	true