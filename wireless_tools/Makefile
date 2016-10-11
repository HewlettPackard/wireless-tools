#
# Basic and crude Makefile...
#

# Targets to build
STATIC=libiw.a
DYNAMIC=libiw.so.22
PROGS= iwconfig iwlist iwpriv iwspy iwgetid

# Composition of the library :
OBJS = iwlib.o

# Define if tools should be built using static or dynamic version of the lib
IWLIBS=$(OBJS)
#IWLIBS=-liw

# Installation directory. By default, go in local.
# Distributions should probably use /usr/sbin, but they probably know better...
INSTALL_DIR= /usr/local/sbin
INSTALL_LIB= /usr/local/lib
INSTALL_INC= /usr/local/include

# This is our header selection. Try to hide the mess and the misery :-(
# Please choose *only one* of the define...

# Kernel headers 2.4.X + Glibc 2.2 - Mandrake 8.0, Debian 2.3, RH 7.1
#HEADERS= -DGLIBC22_HEADERS

# Kernel headers 2.0.X + Glibc 2.0 - Debian 2.0, RH 5
# Kernel headers 2.2.X + Glibc 2.1 - Debian 2.2, RH 6.1
# Kernel headers 2.4.X + Glibc 2.1 - Debian 2.2 upgraded, RH 7.0
HEADERS= -DGLIBC_HEADERS

# Kernel headers 2.2.X + Glibc 2.0 - Debian 2.1
#HEADERS= -DKLUDGE_HEADERS

# Kernel headers 2.0.X + libc5 - old systems
#HEADERS= -DLIBC5_HEADERS

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
libiw.so.22: $(OBJS)
	$(CC) -O2 -shared -o $@ -Wl,-soname,$@ -lm -lc $^

# Compilation of the static library
libiw.a: $(OBJS)
	$(RM) $@
	ar cru $@ $^
	ranlib $@

# So crude but so effective ;-)
install::
	cp $(PROGS) $(INSTALL_DIR)
	cp $(STATIC) $(DYNAMIC) $(INSTALL_LIB)
	echo "Don't forget to fix you /etc/ld.so.conf and run ldconfig."
	cp iwlib.h $(INSTALL_INC)

clean::
	$(RM_CMD) 

realclean::
	$(RM_CMD) 
	$(RM) $(STATIC) $(DYNAMIC) $(PROGS)

depend::
	makedepend -s "# DO NOT DELETE" -- $(INCLUDES) -- $(SRCS)
# DO NOT DELETE
