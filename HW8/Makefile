all: server client

server: hw8-server.c
	gcc -pthread -o server -std=c99 -Wall -O1 -g hw8-server.c

client: hw8-client.c
	gcc -o client -std=c99 -Wall -O1 -g hw8-client.c

clean:
	rm -f shell
