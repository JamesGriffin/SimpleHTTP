all:
	gcc server.c -std=c99 -O3 -lczmq -lzmq -o simplehttp

debug:
	gcc server.c -std=c99 -lczmq -lzmq -o simplehttp -g

