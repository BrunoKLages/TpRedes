all: bin
	gcc -Wall -c common.c
	gcc -Wall client.c common.o -o bin/client
	gcc -Wall server.c common.o -o bin/server -lm

bin: 
	@mkdir -p bin
	
clean: 
	@rm -f common.o bin/client bin/server