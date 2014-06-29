all: client server twitter

client: svr_c.o list.o mail.o
	gcc -pthread svr_c.o -o svr_c -lcurl

svr_c.o: svr_c.c
	gcc -c svr_c.c

list.o: list.c list.h
	gcc -c list.c

server: svr_s.o list.o codes.o mail.o
	gcc -pthread svr_s.o -o svr_s -lcurl

svr_s.o: svr_s.c
	gcc -c svr_s.c

codes.o: codes.c
	gcc -c codes.c

mail.o: mail.c
	gcc -c mail.c -lcurl

twitter: twitter.cpp
	g++ twitter.cpp -o twitter -ltwitcurl

clean:
	rm -rf *.o svr_c svr_s twitter