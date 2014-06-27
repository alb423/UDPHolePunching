CC = gcc
STRIP	= strip
wsdiscovery: main.o util.o
	$(CC) main.o util.o -lpthread -o punching
	$(STRIP) punching
	
main.o:	main.c
	$(CC) -c -Wall main.c 
		
util.o:	util.c
	$(CC) -c -Wall util.c 

clean:	
	rm *.o;rm punching
