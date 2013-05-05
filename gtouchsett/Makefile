LIBS = -X -lm -X -lX11 -X -lXi
PKGS = --pkg gtk+-2.0 --pkg gmodule-2.0
BINDIR = $(DESTDIR)/usr/bin
PROGRAM = gtouchsett
SHAREDIR =  $(DESTDIR)/usr/share/$(PROGRAM)
VALAFILES = src/gtouchsett.vala src/testarea.vala src/settingswindow.vala src/calibration.vala src/xinput.c src/xlib.c src/profiles.c

all: 
	valac $(VALAFILES) -o $(PROGRAM) $(LIBS) $(PKGS)

ccode: 
	valac -C $(VALAFILES) -o $(PROGRAM) $(LIBS) $(PKGS)

release: clean
	valac -X -O2 $(VALAFILES) -o $(PROGRAM) $(LIBS) $(PKGS)

install:
	mkdir -p $(BINDIR)
	install --mode=755 $(PROGRAM) $(BINDIR)/
	mkdir -p $(SHAREDIR)
	install --mode=644 glade/*.glade $(SHAREDIR)/
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/48x48/devices/
	install --mode=644 icons/touchscreen.png -T $(DESTDIR)/usr/share/icons/hicolor/48x48/devices/touchscreen.png
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/24x24/devices/
	install --mode=644 icons/touchscreen24.png -T $(DESTDIR)/usr/share/icons/hicolor/24x24/devices/touchscreen.png
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/22x22/devices/
	install --mode=644 icons/touchscreen22.png -T $(DESTDIR)/usr/share/icons/hicolor/22x22/devices/touchscreen.png
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/16x16/devices/
	install --mode=644 icons/touchscreen16.png -T $(DESTDIR)/usr/share/icons/hicolor/16x16/devices/touchscreen.png
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/32x32/devices/
	install --mode=644 icons/touchscreen32.png -T $(DESTDIR)/usr/share/icons/hicolor/32x32/devices/touchscreen.png
	mkdir -p $(DESTDIR)/usr/share/icons/hicolor/scalable/devices/
	install --mode=644 icons/touchscreen.svg -T $(DESTDIR)/usr/share/icons/hicolor/scalable/devices/touchscreen.svg
	xdg-icon-resource forceupdate
	xdg-desktop-menu install --novendor gtouchsett.desktop

clean:
	if [ -f $(PROGRAM) ]; then rm $(PROGRAM); fi

uninstall:
	rm $(BINDIR)/$(PROGRAM)
	rm $(SHAREDIR)/*.glade
	rmdir $(SHAREDIR)
	rm $(DESTDIR)/usr/share/icons/hicolor/48x48/devices/touchscreen.png
	rm $(DESTDIR)/usr/share/icons/hicolor/32x32/devices/touchscreen.png
	rm $(DESTDIR)/usr/share/icons/hicolor/24x24/devices/touchscreen.png
	rm $(DESTDIR)/usr/share/icons/hicolor/22x22/devices/touchscreen.png
	rm $(DESTDIR)/usr/share/icons/hicolor/16x16/devices/touchscreen.png
	rm $(DESTDIR)/usr/share/icons/hicolor/scalable/devices/touchscreen.svg
	xdg-icon-resource forceupdate
	xdg-desktop-menu uninstall gtouchsett.desktop


