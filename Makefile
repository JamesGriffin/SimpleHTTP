all:
	gcc server.c -std=c99 -O3 -o simplehttp -Wno-unused-result

debug:
	gcc server.c -std=c99 -o simplehttp -g

