#
# Basic and crude Makefile...
#

# Targets to build
STATIC=libiw.a
DYNAMIC=libiw.so.24
PROGS= iwconfig iwlist iwpriv iwspy iwgetid iwevent
MANPAGES=iwconfig.8 iwlist.8 iwpriv.8 iwspy.8 iwgetid.8 iwevent.8

# Composition of the library :
OBJS = iwlib.o

# Define if tools should be built using static or dynamic version of the lib
IWLIB=$(STATIC)
#IWLIB=$(DYNAMIC)

# Standard name for dynamic library so that the dynamic linker can pick it.
# We will just create a symbolic link to the real thing.
DYNAMIC_LINK= libiw.so

# Installation directory. By default, go in local.
# Distributions should probably use /usr/sbin, but they probably know better...
# Don't forget trailing slash to avoid issues

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

RM_CMD = $(RM) *.BAK *.bak *.o *.so ,* *~ *.a *.orig *.rej

#CFLAGS=-O2 -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror
CFLAGS=-O2 -W -Wall -Wstrict-prototypes
XCFLAGS=$(CFLAGS) $(WARN) $(HEADERS) $(WE_HEADER)
PICFLAG=-fPIC

LIBS= -lm

all:: $(STATIC) $(DYNAMIC) $(PROGS)

%: %.o
	$(CC) $(XCFLAGS) -o $@ $^ $(LIBS)
%.o: %.c
	$(CC) $(XCFLAGS) -c $<
%.so: %.c
	$(CC) $(XCFLAGS) $(PICFLAG) -c -o $@ $<

iwconfig: iwconfig.o $(IWLIB)

iwlist: iwlist.o $(IWLIB)

iwpriv: iwpriv.o $(IWLIB)

iwspy: iwspy.o $(IWLIB)

iwgetid: iwgetid.o

iwevent: iwevent.o $(IWLIB)

macaddr: macaddr.o $(IWLIB)

# Compilation of the dynamic library
$(DYNAMIC): $(OBJS:.o=.so)
	$(CC) -shared -o $@ -Wl,-soname,$@ -lm -lc $^

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
	ln -s $(INSTALL_LIB)/$(DYNAMIC) $(INSTALL_LIB)/$(DYNAMIC_LINK)
	echo "Don't forget to add $(INSTALL_LIB) to /etc/ld.so.conf, and run ldconfig."
	install -m 644 iwlib.h $(INSTALL_INC)
	install -m 644 $(MANPAGES) $(INSTALL_MAN)/man8/

clean::
	$(RM_CMD) 

realclean::
	$(RM_CMD) 
	$(RM) $(STATIC) $(DYNAMIC) $(PROGS) macaddr

depend::
	makedepend -s "# DO NOT DELETE" -- $(INCLUDES) -- $(SRCS)
# DO NOT DELETE
