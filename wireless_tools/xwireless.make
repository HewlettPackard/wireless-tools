# Makefile for Xwireless
CC = gcc
RM = rm -f

RM_CMD = $(RM) *.BAK *.bak *.o ,* *~ *.a

INCLUDES =	$(SYS_INCLUDES) $(LOCAL_INCLUDES)
LIBRARIES =	$(LOCAL_LIBRARIES) $(SYS_LIBRARIES)
LIBPATH = $(SYS_LIBPATH) $(LOCAL_LIBPATH)

#
# System stuff
#
SYS_INCLUDES =	-I/usr/include  -I/usr/X11R6/include
SYS_LIBRARIES = -lXaw -lXmu -lXt -lXext  -lSM -lICE -lX11	
SYS_LIBPATH = -L/usr/lib -L/usr/local/lib -L/usr/X11R6/lib

#
# Local stuff
#
LOCAL_INCLUDES	= 
LOCAL_LIBRARIES	= 
LOCAL_LIBPATH = 

XTRACFLAGS=-Wall -pipe -I.

# Uncomment these lines for a production compile
#CFLAGS=-O3 -m486 -fomit-frame-pointer
#LDFLAGS=-s
CFLAGS=-g

#
# Files to make
#
PROGS=xwireless

SRCS = $(PROGS).c
OBJS = $(PROGS).o

all:: $(PROGS)

xwireless: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(INCLUDES) $(LIBPATH) $(LIBRARIES) 

.c.o:
	$(CC) $(CFLAGS) $(XTRACFLAGS) -c $(INCLUDES) -DNARROWPROTO $<

clean::
	$(RM_CMD) 

depend::
	makedepend -s "# DO NOT DELETE" -- $(INCLUDES) -- $(SRCS)
# DO NOT DELETE
