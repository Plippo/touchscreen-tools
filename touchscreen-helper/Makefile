CC = gcc
OBJECTS = touchscreen-helper.o profiles.o
LIBS = -lX11 -lXrandr -lpthread -lXi
CFLAGS = -Wall -O2
BINDIR = $(DESTDIR)/usr/bin
PROGRAM = touchscreen-helper

all: $(OBJECTS)
	$(CC) -o $(PROGRAM) $(OBJECTS) $(LIBS)

%.o: src/%.c
	$(CC) -c $(CFLAGS) $<

install:
	mkdir -p $(BINDIR)
	mkdir -p $(DESTDIR)/etc/X11/Xsession.d/
	install --mode=755 $(PROGRAM) $(BINDIR)/
	install --mode=755 98touchscreen-helper $(DESTDIR)/etc/X11/Xsession.d/

clean:
	rm -f *.o $(PROGRAM)

uninstall:
	rm $(BINDIR)/$(PROGRAM)
	rm $(DESTDIR)/etc/X11/Xsession.d/98touchscreen-helper

