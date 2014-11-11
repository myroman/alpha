include Make.defines

all: client server test

get_ifi_info_plus.o: get_ifi_info_plus.c
	${CC} ${CFLAGS} -c get_ifi_info_plus.c ${UNP}

client: client.o
	${CC} ${FLAGS} -o $@ client.o ${LIBS}
server: server.o
	${CC} ${FLAGS} -o $@ server.o ${LIBS}
test: test.o
	${CC} ${FLAGS} -o $@ test.o ${LIBS}
test.o: test.c
	${CC} ${FLAGS} -c test.c ${UNP}
clean:
	rm *.o client server test