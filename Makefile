# buffalo - buffalo buffalo buffalo buffalo buffalo buffalo buffalo
# See LICENSE file for copyright and license details.

include config.mk

SRC = buffalo.c codes.c
OBJ = ${SRC:.c=.o}

all: options buffalo

options:
	@echo buffalo build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -g -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	@echo creating $@ from config.def.h
	@cp config.def.h $@

buffalo: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${LDFLAGS} ${OBJ}

clean:
	@echo cleaning
	@rm -f buffalo ${OBJ} buffalo-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p buffalo-${VERSION}
	@cp -R LICENSE Makefile config.mk config.def.h \
		README TODO buffalo.1 ${SRC} buffalo-${VERSION}
	@tar -cf buffalo-${VERSION}.tar buffalo-${VERSION}
	@gzip buffalo-${VERSION}.tar
	@rm -rf buffalo-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f buffalo ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/buffalo
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@sed "s/VERSION/${VERSION}/g" < buffalo.1 > ${DESTDIR}${MANPREFIX}/man1/buffalo.1
	@chmod 644 ${DESTDIR}${MANPREFIX}/man1/buffalo.1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/buffalo
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1
	@rm -f ${DESTDIR}${MANPREFIX}/man1/buffalo.1

.PHONY: all options clean dist install uninstall
