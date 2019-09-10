
CC ?= cc

# Possible platforms are SUN, HPUX, DEC, SGI, AIX, Linux and MSDOS
# The main definitions are for 32/16 bits and for byte order, the 
# default is big endian.  You can do -D__TNEF_BYTE_ORDER 1234 for little
# endian
PLATFORM=-D___TNEF_BYTE_ORDER=4321 
#CFLAGS = -O2 -ggdb
CFLAGS = -fPIC -O2  -Wall
OBJS=logger.o tnef.o rtf_decompress.o

all: opentnef libopentnef.so.1

.c.o:
	${CC} ${LDFLAGS} ${CFLAGS} $(PLATFORM) -c $*.c

opentnef: opentnef.c $(OBJS)
	$(CC) $(CFLAGS) ${LDFLAGS} $(PLATFORM) $(OBJS) opentnef.c -o opentnef

libopentnef.so.1: ${OBJS}
	$(CC) $(CFLAGS) ${LDFLAGS} $(PLATFORM) $(OBJS) -shared -Wl,-soname,libopentnef.so.1 -o libopentnef.so.1

install: opentnef libopentnef.so.1
	install -d ${DESTDIR}/usr/bin
	install -C opentnef ${DESTDIR}/usr/bin
	install -d ${DESTDIR}/usr/lib
	install -C libopentnef.so* ${DESTDIR}/usr/lib
	install -d ${DESTDIR}/usr/include
	install -C tnef_api.h ${DESTDIR}/usr/include
	rm -f ${DESTDIR}/usr/lib/libopentnef.so
	ln -s libopentnef.so.1 ${DESTDIR}/usr/lib/libopentnef.so

clean:
	rm -f opentnef *.so *.so.* *.o *.~[ch]
