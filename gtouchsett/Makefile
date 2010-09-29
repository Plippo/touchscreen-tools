LIBS = -X -lm -X -lX11 -X -lXi
PKGS = --pkg gtk+-2.0 --pkg gmodule-2.0
BINDIR = $(DESTDIR)/usr/bin
PROGRAM = gtouchsett
SHAREDIR = /usr/share/$(PROGRAM)
VALAFILES = src/gtouchsett.vala src/testarea.vala src/settingswindow.vala src/calibration.vala src/xinput.c src/xlib.c src/profiles.c

all: 
	valac $(VALAFILES) -o $(PROGRAM) $(LIBS) $(PKGS)

release: clean
	valac -X -O2 $(VALAFILES) -o $(PROGRAM) $(LIBS) $(PKGS)

install:
	install --mode=755 $(PROGRAM) $(BINDIR)/
	mkdir -p $(SHAREDIR)
	install --mode=644 glade/*.glade $(SHAREDIR)/
	install --mode=644 icons/touchscreen.png -T /usr/share/icons/hicolor/48x48/devices/touchscreen.png
	install --mode=644 icons/touchscreen24.png -T /usr/share/icons/hicolor/24x24/devices/touchscreen.png
	install --mode=644 icons/touchscreen22.png -T /usr/share/icons/hicolor/22x22/devices/touchscreen.png
	install --mode=644 icons/touchscreen16.png -T /usr/share/icons/hicolor/16x16/devices/touchscreen.png
	install --mode=644 icons/touchscreen32.png -T /usr/share/icons/hicolor/32x32/devices/touchscreen.png
	install --mode=644 icons/touchscreen.svg -T /usr/share/icons/hicolor/scalable/devices/touchscreen.svg
	xdg-icon-resource forceupdate
	xdg-desktop-menu install --novendor gtouchsett.desktop

clean:
	rm $(PROGRAM)

uninstall:
	rm $(BINDIR)/$(PROGRAM)
	rm $(SHAREDIR)/*.glade
	rmdir $(SHAREDIR)
	rm /usr/share/icons/hicolor/48x48/devices/touchscreen.png
	rm /usr/share/icons/hicolor/32x32/devices/touchscreen.png
	rm /usr/share/icons/hicolor/24x24/devices/touchscreen.png
	rm /usr/share/icons/hicolor/22x22/devices/touchscreen.png
	rm /usr/share/icons/hicolor/16x16/devices/touchscreen.png
	rm /usr/share/icons/hicolor/scalable/devices/touchscreen.svg
	xdg-icon-resource forceupdate
	xdg-desktop-menu uninstall gtouchsett.desktop


