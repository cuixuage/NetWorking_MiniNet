all: client server

client: client.c
	gcc -Wall -g client.c -o client -lpthread

server: server.c
	gcc -Wall -g server.c -o server -lpthread

clean:
	@rm client server
