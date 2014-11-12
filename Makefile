include Make.defines

all: client server test get_hw_addrs.o prhwaddrs.o odrProc
	${CC} -o prhwaddrs prhwaddrs.o get_hw_addrs.o ${LIBS}

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${FLAGS} -c get_hw_addrs.c

prhwaddrs.o: prhwaddrs.c
	${CC} ${FLAGS} -c prhwaddrs.c

get_ifi_info_plus.o: get_ifi_info_plus.c
	${CC} ${CFLAGS} -c get_ifi_info_plus.c ${UNP}

client: client.o oapi.o
	${CC} ${FLAGS} -o $@ client.o oapi.o ${LIBS}
server: server.o oapi.o
	${CC} ${FLAGS} -o $@ server.o oapi.o ${LIBS}
oapi.o: oapi.c
	${CC} ${FLAGS} -c oapi.c ${UNP}	
test.o: test.c
	${CC} ${FLAGS} -c test.c ${UNP}
odrProc: odrProc.o odrImpl.o
	${CC} ${FLAGS} -o $@ odrProc.o odrImpl.o ${LIBS}
odrImpl.o: odrImpl.c
	${CC} ${FLAGS} -c odrImpl.c ${UNP}


test: test.o
	${CC} ${FLAGS} -o $@ test.o ${LIBS}
clean:
	rm *.o client server test odrProc