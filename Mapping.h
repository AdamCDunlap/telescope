//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: Mapping.h 158 2011-02-22 20:05:07Z mitrandir $

// Mapping - a single key binding

#ifndef __TELESCOPE__MAPPING_H
#define __TELESCOPE__MAPPING_H

#include <X11/Xlib.h>


class TeleWindow;

class Mapping
{
    public:
        enum Event
        {
            Press,
            GlobalPress
        };

        enum Type
        {
            Shell,
            Internal
        };

    private:
        Event _event;
        KeyCode _keyCode;
        Type _type;
        char *_action;

        Mapping();

        Mapping(Event event, KeyCode keyCode, Type type, const char *action);

    public:
        static Mapping* parseMappingLine(Display *dpy, const char *line, const char **error);
        ~Mapping();

        void handleEvent(TeleWindow *teleWindow, Mapping::Event event, KeyCode keyCode);
        void execute(TeleWindow *teleWindow);
};


#endif
