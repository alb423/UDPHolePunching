CROSS_COMPILE=
CC	= $(CROSS_COMPILE)gcc
STRIP	= $(CROSS_COMPILE)strip
AR	= $(CROSS_COMPILE)ar

CFLAGS  = -g -Wall -Wno-format-security
OBJ     = util.o coordinator.o punching.o sqlite3.o sqlite3_util.o
SOURCES = util.c punching.c coordinator.c sqlite3.c sqlite3_util.c
LIBS    = libtwp2p.a -lpthread -ldl
   
all: libtwp2p.a punching
   
libtwp2p.a: $(OBJ)
	$(AR) cr libtwp2p.a $(OBJ) 

punching: main.o
	$(CC) $(CFLAGS) -o $@ main.o $(LIBS)
	cp punching coordinator

clean:	
	rm *.o;rm libtwp2p.a punching coordinator
