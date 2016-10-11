#
# Basic and crude Makefile...
#

# Targets to build
STATIC=libiw.a
DYNAMIC=libiw.so.23
PROGS= iwconfig iwlist iwpriv iwspy iwgetid
MANPAGES=iwconfig.8 iwlist.8 iwpriv.8 iwspy.8

# Composition of the library :
OBJS = iwlib.o

# Define if tools should be built using static or dynamic version of the lib
IWLIBS=$(OBJS)
#IWLIBS=-liw

# Installation directory. By default, go in local.
# Distributions should probably use /usr/sbin, but they probably know better...
INSTALL_DIR= /usr/local/sbin/
INSTALL_LIB= /usr/local/lib/
INSTALL_INC= /usr/local/include/
INSTALL_MAN= /usr/local/man

# Header selection is now supposed to be automatic...

# Use private copy of Wireless Extension definition instead of the
# system wide one in /usr/include/linux. Use with care.
# Can be used to create multiple versions of the tools on the same system
# for multiple kernels or get around broken distributions.
#WE_HEADER= -DPRIVATE_WE_HEADER
WE_HEADER=

# ------------ End of config --------------

CC = gcc
RM = rm -f

RM_CMD = $(RM) *.BAK *.bak *.o ,* *~ *.a

CFLAGS=-O2 -Wall $(HEADERS) $(WE_HEADER)

LIBS=$(IWLIBS) -lm

all:: $(STATIC) $(DYNAMIC) $(PROGS)

.c.o:
	$(CC) $(CFLAGS) -c $<

iwconfig: iwconfig.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

iwlist: iwlist.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

iwpriv: iwpriv.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

iwspy: iwspy.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

iwgetid: iwgetid.o
	$(CC) $(CFLAGS) -o $@ $^

# Compilation of the dynamic library
$(DYNAMIC): $(OBJS)
	$(CC) -O2 -shared -o $@ -Wl,-soname,$@ -lm -lc $^

# Compilation of the static library
$(STATIC): $(OBJS)
	$(RM) $@
	ar cru $@ $^
	ranlib $@

# So crude but so effective ;-)
# Less crude thanks to many contributions ;-)
install::
	install -m 755 $(PROGS) $(INSTALL_DIR)
	install -m 644 $(STATIC) $(INSTALL_LIB)
	install -m 755 $(DYNAMIC) $(INSTALL_LIB)
	echo "Don't forget to fix your /etc/ld.so.conf and run ldconfig."
	install -m 644 iwlib.h $(INSTALL_INC)
	install -m 644 $(MANPAGES) $(INSTALL_MAN)/man8/

clean::
	$(RM_CMD) 

realclean::
	$(RM_CMD) 
	$(RM) $(STATIC) $(DYNAMIC) $(PROGS)

depend::
	makedepend -s "# DO NOT DELETE" -- $(INCLUDES) -- $(SRCS)
# DO NOT DELETE
