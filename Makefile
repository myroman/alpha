include Make.defines

all: client server

get_ifi_info_plus.o: get_ifi_info_plus.c
	${CC} ${CFLAGS} -c get_ifi_info_plus.c ${UNP}

client: client.o
	${CC} ${FLAGS} -o client client.o ${LIBS}
server: server.o
	${CC} ${FLAGS} -o server server.o ${LIBS}

clean:
	rm *.o client