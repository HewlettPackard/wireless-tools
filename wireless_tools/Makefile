CC = gcc
RM = rm -f

RM_CMD = $(RM) *.BAK *.bak *.o ,* *~ *.a

CFLAGS=-O2 -Wall

PROGS=iwconfig iwpriv iwspy
LIBS=-lm

all:: $(PROGS)

.c.o:
	$(CC) $(CFLAGS) -c $<

iwconfig: iwconfig.o iwcommon.o
	gcc -O2 -o $@ $^ $(LIBS)

iwpriv: iwpriv.o iwcommon.o
	gcc -O2 -o $@ $^ $(LIBS)

iwspy: iwspy.o iwcommon.o
	gcc -O2 -o $@ $^ $(LIBS)

clean::
	$(RM_CMD) 

depend::
	makedepend -s "# DO NOT DELETE" -- $(INCLUDES) -- $(SRCS)
# DO NOT DELETE
