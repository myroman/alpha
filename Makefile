include Make.defines

all: test

get_ifi_info_plus.o: get_ifi_info_plus.c
	${CC} ${CFLAGS} -c get_ifi_info_plus.c ${UNP}

test: test.o
	${CC} ${FLAGS} -o test test.o ${LIBS}
test.o: test.c
	${CC} ${FLAGS} -c test.c ${UNP}

clean:
	rm *.o test
