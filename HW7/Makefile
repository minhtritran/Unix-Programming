all: server client

server: hw7-server.c
	gcc -o server -std=c99 -Wall -O1 -g hw7-server.c

client: hw7-client.c
	gcc -o client -std=c99 -Wall -O1 -g hw7-client.c

clean:
	rm -f shell
