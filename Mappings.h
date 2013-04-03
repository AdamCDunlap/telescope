//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: Mappings.h 158 2011-02-22 20:05:07Z mitrandir $

// Mappings - handling keyboard bindings

#ifndef __TELESCOPE__MAPPINGS_H
#define __TELESCOPE__MAPPINGS_H

#include <X11/Xlib.h>

#include "LinkedList.h"
#include "Mapping.h"

class TeleWindow;

class Mappings
{
    private:
        static Mappings *_instance;

        Display *_dpy;

        LinkedList<Mapping*> _mappings;


        void loadMappings();

        void parseLine(const char *line);

    public:
        Mappings(Display *dpy);
        ~Mappings();

        Mappings* instance() { return _instance; }


        void handleEvent(TeleWindow *teleWindow, Mapping::Event event, KeyCode keyCode);
};


#endif
