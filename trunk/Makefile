PRODUCT = id3
VERSION = 0.13

SHELL = /bin/sh

CC = gcc
CFLAGS = -g -O2
LDFLAGS = 
LIBS = 
DEFS =  
INSTALL = /usr/bin/install -c

# Installation directories
prefix = ${DESTDIR}/usr
exec_prefix = ${prefix}
mandir = ${prefix}/share/man/man1
bindir = ${exec_prefix}/bin

INCL = 
SRCS = id3.c
OBJS = $(SRCS:.c=.o)

.SUFFIXES: .c .o

.c.o:
	$(CC) $(DEFS) $(CFLAGS) -c $<

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f *~ *.o core $(PRODUCT)

install: $(PRODUCT)
	$(INSTALL) -d -m 755 $(bindir)
	$(INSTALL) -s -m 755 -o 0 $(PRODUCT) $(bindir)
	$(INSTALL) -d -m 755 $(mandir)
	$(INSTALL) -m 644 -o 0 $(PRODUCT).1 $(mandir)
