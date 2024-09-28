CC = gcc

CFLAGS = -Wall -Werror

all: aesdsocket

aesdsocket.o: aesdsocket.c
	${CC} ${CFLAGS} -c aesdsocket.c

aesdsocket: aesdsocket.o
	${CC} ${LDFLAGS} aesdsocket.o -o aesdsocket

clean:
	rm -f aesdsocket aesdsocket.o
