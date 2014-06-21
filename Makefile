all: client server

client: svr_c.o list.o
	gcc -pthread svr_c.o -o svr_c

svr_c.o: svr_c.c
	gcc -c svr_c.c

list.o: list.c list.h
	gcc -c list.c

server: svr_s.o
	gcc -pthread svr_s.o -o svr_s

svr_s.o: svr_s.c
	gcc -c svr_s.c

clean:
	rm -rf *.o svr_c svr_s