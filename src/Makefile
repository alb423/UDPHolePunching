CROSS_COMPILE=
CC	= $(CROSS_COMPILE)gcc
STRIP	= $(CROSS_COMPILE)strip
AR	= $(CROSS_COMPILE)ar

CFLAGS  = -g -Wall -Wno-format-security
OBJ     = util.o coordinator.o client.o
SOURCES = util.c client.c coordinator.c
LIBS    = libtwp2p.a -lpthread
   
all: libtwp2p.a punching
   
libtwp2p.a: $(OBJ)
	$(AR) cr libtwp2p.a $(OBJ) 

punching: main.o
	$(CC) $(CFLAGS) -o $@ main.o $(LIBS)
	cp punching coordinator

clean:	
	rm *.o;rm libtwp2p.a punching coordinator
