CC = gcc
STRIP	= strip
punching: main.o util.o coordinator.o
	$(CC) main.o util.o coordinator.o -lpthread -o punching
	cp punching coordinator
   
main.o:	main.c
	$(CC) -c -Wall main.c 
		
util.o:	util.c
	$(CC) -c -Wall util.c 

coordinator.o:	coordinator.c
	$(CC) -c -Wall coordinator.c 
	
clean:	
	rm *.o;rm punching coordinator
