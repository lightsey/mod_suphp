##############################################
## Makefile for suPHP                       ##
##############################################

SUPHP_INSTALL = /usr/sbin/suphp

CC = gcc
CFLAGS = -c -Wall
LD = gcc
LDFLAGS = -o

suphp: suphp.o filesystem.o check.o error.o log.o
	${LD} ${LDFLAGS} suphp suphp.o filesystem.o check.o error.o log.o

suphp.o: suphp.c suphp.h
	${CC} ${CFLAGS} suphp.c

filesystem.o: filesystem.c suphp.h
	${CC} ${CFLAGS} filesystem.c

check.o: check.c suphp.h
	${CC} ${CFLAGS} check.c

error.o: error.c suphp.h
	${CC} ${CFLAGS} error.c

log.o: log.c suphp.h
	${CC} ${CFLAGS} log.c

suphp.h: filesystem.h check.h error.h log.h config.h
	touch suphp.h

install: suphp
	if [ $$UID = 0 ]; then \
	 cp suphp ${SUPHP_INSTALL}; \
	else \
	 echo -e "You need to be root to install suPHP."; \
	fi

clean:
	rm *.o
	rm suphp

rmbackups:
	rm *~
