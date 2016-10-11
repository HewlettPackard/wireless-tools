##
## Please check the configurion parameters below
##

## Installation directory. By default, go in /usr/local
## Distributions should probably use /usr, but they probably know better...
PREFIX = /usr/local

## Compiler to use
CC = gcc

## Uncomment this to build against this kernel
# KERNEL_SRC = /usr/src/linux

## Uncomment this to force a particular version of wireless extensions.
## This would use private copy of Wireless Extension definition instead
## of the system wide one in /usr/include/linux. Use with care.
## Can be used to create multiple versions of the tools on the same system
## for multiple kernels or get around broken distributions.
# FORCE_WEXT_VERSION = 14

## Uncomment this to build tools using dynamic version of the library
# BUILD_SHARED = y

## Uncomment this to build without using libm (less efficient)
## This is mostly useful for embedded platforms
# BUILD_NOLIBM = y

# ***************************************************************************
# ***** Most users should not need to change anything beyond this point *****
# ***************************************************************************

# Targets to build
STATIC=libiw.a
DYNAMIC=libiw.so.25
PROGS= iwconfig iwlist iwpriv iwspy iwgetid iwevent
MANPAGES8=iwconfig.8 iwlist.8 iwpriv.8 iwspy.8 iwgetid.8 iwevent.8
MANPAGES7=wireless.7

# Composition of the library :
OBJS = iwlib.o

# Select library to link tool with
ifdef BUILD_SHARED
  IWLIB=$(DYNAMIC)
else
  IWLIB=$(STATIC)
endif

# Standard name for dynamic library so that the dynamic linker can pick it.
# We will just create a symbolic link to the real thing.
DYNAMIC_LINK= libiw.so

# Install directories
INSTALL_DIR= $(PREFIX)/sbin/
INSTALL_LIB= $(PREFIX)/lib/
INSTALL_INC= $(PREFIX)/include/
INSTALL_MAN= $(PREFIX)/man/

# Use local header if the version of wireless extensions is specified
ifdef FORCE_WEXT_VERSION
  WEXT_FLAG = -DWEXT_HEADER=\"wireless.$(FORCE_WEXT_VERSION).h\"
endif

RM = rm -f

RM_CMD = $(RM) *.BAK *.bak *.o *.so ,* *~ *.a *.orig *.rej

ifdef KERNEL_SRC
  KERNEL_INCLUDES = -I $(KERNEL_SRC)/include
endif

# Do we want to build with or without libm ?
ifdef BUILD_NOLIBM
  LIBS=
  WELIB_FLAG = -DWE_NOLIBM=y
else
  LIBS= -lm
endif

#CFLAGS=-O2 -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror
CFLAGS=-O2 -W -Wall -Wstrict-prototypes
XCFLAGS=$(CFLAGS) $(WARN) $(HEADERS) $(WEXT_FLAG) $(WELIB_FLAG) $(KERNEL_INCLUDES)
PICFLAG=-fPIC

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
	install -m 755 -d $(INSTALL_DIR)
	install -m 755 $(PROGS) $(INSTALL_DIR)
	install -m 755 -d $(INSTALL_LIB)
	install -m 644 $(STATIC) $(INSTALL_LIB)
	install -m 755 $(DYNAMIC) $(INSTALL_LIB)
	ln -sfn $(DYNAMIC) $(INSTALL_LIB)/$(DYNAMIC_LINK)
	echo "Don't forget to add $(INSTALL_LIB) to /etc/ld.so.conf, and run ldconfig."
	install -m 755 -d $(INSTALL_INC)
	install -m 644 iwlib.h $(INSTALL_INC)
	install -m 755 -d $(INSTALL_MAN)/man8/
	install -m 644 $(MANPAGES8) $(INSTALL_MAN)/man8/
	install -m 755 -d $(INSTALL_MAN)/man7/
	install -m 644 $(MANPAGES7) $(INSTALL_MAN)/man7/

clean::
	$(RM_CMD) 

realclean::
	$(RM_CMD) 
	$(RM) $(STATIC) $(DYNAMIC) $(PROGS) macaddr

uninstall::
	for f in $(PROGS); do \
	  $(RM) $(INSTALL_DIR)/$$f; \
	done
	$(RM) $(INSTALL_LIB)/$(STATIC)
	$(RM) $(INSTALL_LIB)/$(DYNAMIC)
	$(RM) $(INSTALL_LIB)/$(DYNAMIC_LINK)
	$(RM) $(INSTALL_INC)/iwlib.h
	for f in $(MANPAGES8); do \
	  $(RM) $(INSTALL_MAN)/man8/$$f; \
	for f in $(MANPAGES7); do \
	  $(RM) $(INSTALL_MAN)/man7/$$f; \
	done

depend::
	makedepend -s "# DO NOT DELETE" -- $(INCLUDES) -- $(SRCS)
# DO NOT DELETE
