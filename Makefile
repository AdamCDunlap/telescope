##
## Telescope - graphical task switcher
##
## (c) Ilya Skriblovsky, 2010
## <Ilya.Skriblovsky@gmail.com>
##

## $Id: Makefile 177 2011-03-01 11:38:13Z mitrandir $

all: telescope

#DEFINES = -DDESKTOP -DDBUS
DEFINES = -DMAEMO4 -DDBUS

LAUNCHER = 1

SOURCES = TeleWindow.cpp    \
          Main.cpp          \
          XTools.cpp        \
          Thumbnail.cpp     \
          Settings.cpp      \
          Mapping.cpp       \
          Mappings.cpp      \
          Resources.cpp     \
          DBus.cpp          \
          XEventLoop.cpp    \
          Image.cpp


ifeq ($(LAUNCHER),1)
    DEFINES += -DLAUNCHER

    SOURCES += \
        MenuReader.cpp       \
        LauncherWindow.cpp   \
        SectionList.cpp      \
        Section.cpp          \
        Application.cpp

    SHAREFILES += \
         text-background.png        \
         indicator.png              \
         navigation.png             \
         transparent.png            \
         panel-icons/panel.png      \
         panel-icons/panel-focus-left.png   \
         panel-icons/panel-focus-right.png  \
         panel-icons/panel-focus-middle.png \
         category-icons
endif


DEPS = x11 xcomposite xdamage xrender imlib2 xft dbus-1 glib-2.0

SHAREFILES += header-left.png    \
              header-right.png   \
              header-middle.png  \
              header-left-selected.png   \
              header-right-selected.png  \
              header-middle-selected.png \
              broken-pattern.png         \
              install-notice.txt

CONFFILES = telescope.conf telescope.keys

CFLAGS += -Wall -Werror -O2 $(DEFINES) `pkg-config --cflags $(DEPS)` -pthread
# CFLAGS += -Wall -Werror -g $(DEFINES) `pkg-config --cflags $(DEPS)` -pthread


OBJS = $(SOURCES:%.cpp=%.o)

telescope: $(OBJS)
	g++ -pthread $^ -o $@ `pkg-config --libs $(DEPS)`

.cpp.o:
	g++ -c $(CFLAGS) $< -o $@


depend: $(SOURCES)
	g++ -M -MM -MG -MP $(CFLAGS) $(SOURCES) >$@


include depend


clean:
	rm -f *.o telescope depend *~


install: telescope telescope-svc $(SHAREFILES) $(CONFFILES)
	cp telescope $(DESTDIR)/usr/bin/
	cp telescope-svc $(DESTDIR)/etc/init.d/
	mkdir -p $(DESTDIR)/usr/share/telescope
	cp -r $(SHAREFILES) $(DESTDIR)/usr/share/telescope/
	cp $(CONFFILES) $(DESTDIR)/etc/
