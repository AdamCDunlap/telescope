DEPS = gtk+-2.0 libhildondesktop hildon-1 dbus-1

DEFINES =

SOURCES = TelescopeApplet.cpp 

CFLAGS += -Wall -Werror -O2 $(DEFINES) `pkg-config --cflags $(DEPS)`


.c.o:
	gcc -c $(CFLAGS) $< -o $@



all: libtelescopeapplet.so liblaunchersbapplet.so liblaunchertnapplet.so


libtelescopeapplet.so: TelescopeApplet.o
	gcc $^ -o $@ -shared `pkg-config --libs $(DEPS)`

liblaunchersbapplet.so: LauncherStatusBarApplet.o
	gcc $^ -o $@ -shared `pkg-config --libs $(DEPS)`

liblaunchertnapplet.so: LauncherTaskNavigatorApplet.o
	gcc $^ -o $@ -shared `pkg-config --libs $(DEPS)`

clean:
	rm -rf libtelescopeapplet.so liblaunchersbapplet.so liblaunchertnapplet.so *.o *~

install: libtelescopeapplet.so liblaunchersbapplet.so liblaunchertnapplet.so
	cp libtelescopeapplet.so $(DESTDIR)/usr/lib/hildon-desktop/
	cp liblaunchersbapplet.so $(DESTDIR)/usr/lib/hildon-desktop/
	cp liblaunchertnapplet.so $(DESTDIR)/usr/lib/hildon-desktop/
	cp TelescopeApplet.desktop $(DESTDIR)/usr/share/applications/hildon-status-bar/
	cp launcher-statusbar.desktop $(DESTDIR)/usr/share/applications/hildon-status-bar/
	cp launcher-navigator.desktop $(DESTDIR)/usr/share/applications/hildon-navigator/
	cp telescope-applet.png $(DESTDIR)/usr/share/telescope-applet/
	cp telescope-applet-pressed.png $(DESTDIR)/usr/share/telescope-applet/
