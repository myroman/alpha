include Make.defines


all: client server test get_hw_addrs.o prhwaddrs.o odrProc portPath bidTable
	${CC} -o prhwaddrs prhwaddrs.o get_hw_addrs.o ${LIBS}

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${FLAGS} -c get_hw_addrs.c

prhwaddrs.o: prhwaddrs.c
	${CC} ${FLAGS} -c prhwaddrs.c

get_ifi_info_plus.o: get_ifi_info_plus.c
	${CC} ${CFLAGS} -c get_ifi_info_plus.c ${UNP}

client: client.o oapi.o misc.o get_hw_addrs.o payloadHdr.o
	${CC} ${FLAGS} -o $@ client.o oapi.o misc.o get_hw_addrs.o payloadHdr.o ${LIBS}
server: server.o oapi.o misc.o payloadHdr.o
	${CC} ${FLAGS} -o $@ server.o oapi.o misc.o payloadHdr.o ${LIBS}
oapi.o: oapi.c
	${CC} ${FLAGS} -c oapi.c ${UNP}	
test.o: test.c
	${CC} ${FLAGS} -c test.c ${UNP}
misc.o: misc.c
	${CC} ${FLAGS} -c misc.c ${UNP}	
odrProc: odrProc.o odrImpl.o misc.o oapi.o get_hw_addrs.o payloadHdr.o routingTable.o
	${CC} ${FLAGS} -o $@ odrProc.o odrImpl.o misc.o oapi.o get_hw_addrs.o payloadHdr.o routingTable.o ${LIBS}
odrImpl.o: odrImpl.c
	${CC} ${FLAGS} -c odrImpl.c ${UNP}

routingTable.o: routingTable.c
	${CC} ${FLAGS} -c routingTable.c ${UNP}

portPath.o: portPath.c
	${CC} ${FLAGS} -c portPath.c ${UNP}
portPath: portPath.o
	${CC} ${FLAGS} -o $@ portPath.o ${LIBS}

bidTable.o: bidTable.c
	${CC} ${FLAGS} -c bidTable.c ${UNP}
bidTable: bidTable.o misc.o
	${CC} ${FLAGS} -o $@ misc.o bidTable.o ${LIBS}

payloadHdr.o: payloadHdr.c
	${CC} ${FLAGS} -c payloadHdr.c ${UNP}
test: test.o
	${CC} ${FLAGS} -o $@ test.o ${LIBS}

clean:
	rm *.o client server test odrProc portPath
