//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: Mapping.cpp 158 2011-02-22 20:05:07Z mitrandir $

#include "Mapping.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "TeleWindow.h"


Mapping::Mapping(Event event, KeyCode keyCode, Type type, const char *action)
    : _event(event), _keyCode(keyCode), _type(type)
{
    _action = strdup(action);
}


Mapping::~Mapping()
{
    free(_action);
}



// Line syntax is:
//   event(keysym) : actiontype(action)
//
// Parts are:
// event: press | globalpress
// keysym: X11 keysym
// actiontype: shell | internal
// action: shell command | internal command
//
// Spaces are permitted only around colon (and inside
// shell commands, of course)
Mapping* Mapping::parseMappingLine(Display *dpy, const char *line, const char **error)
{
    *error = 0;

    const char *eventstart = &line[strspn(line, " \t")];

    if (*eventstart == '\n') return 0;
    if (*eventstart == '\0') return 0;
    if (*eventstart == '#') return 0;

// event
    const char *eventend = eventstart + strcspn(eventstart, "(");
    char *event = strndup(eventstart, eventend - eventstart);

// keysym
    const char *keysymstart = eventend + 1;
    const char *keysymend = keysymstart + strcspn(keysymstart, ")");
    char *keysym = strndup(keysymstart, keysymend - keysymstart);

// colon
    const char *colonstart = keysymend + 1;
    const char *colonend = colonstart + strspn(colonstart, " :");

// actiontype
    const char *actiontypestart = colonend;
    const char *actiontypeend = actiontypestart + strcspn(actiontypestart, "(");
    char *actiontype = strndup(actiontypestart, actiontypeend - actiontypestart);

// action
    const char *actionstart = actiontypeend + 1;
    const char *actionend = actionstart + strcspn(actionstart, ")");
    char *action = strndup(actionstart, actionend - actionstart);

//    printf("event:\t%s\n", event);
//    printf("keysym:\t%s\n", keysym);
//    printf("actiontype:\t%s\n", actiontype);
//    printf("action:\t%s\n", action);
//    printf("\n");

    KeySym keySym = XStringToKeysym(keysym);
    KeyCode keyCode = XKeysymToKeycode(dpy, keySym);


    Mapping *result = 0;

    Event ev;
    if (strcmp(event, "press") == 0)
        ev = Press;
    else if (strcmp(event, "globalpress") == 0)
        ev = GlobalPress;
    else
    {
        *error = "Invalid event";
        goto finish;
    }

    if (keyCode == 0)
    {
        *error = "Invalid keysym";
        goto finish;
    }

    Type type;
    if (strcmp(actiontype, "shell") == 0)
        type = Shell;
    else if (strcmp(actiontype, "internal") == 0)
        type = Internal;
    else
    {
        *error = "Invalid action type";
        goto finish;
    }

    result = new Mapping(ev, keyCode, type, action);

    if (ev == GlobalPress)
    {
        Window rootWindow = RootWindow(dpy, DefaultScreen(dpy));
        XGrabKey(dpy, keyCode, 0, rootWindow, False, GrabModeAsync, GrabModeAsync);
    }

finish:
    free(event);
    free(keysym);
    free(actiontype);
    free(action);

    return result;
}


void Mapping::handleEvent(TeleWindow *teleWindow, Mapping::Event event, KeyCode keyCode)
{
    if (_event == event && _keyCode == keyCode)
        execute(teleWindow);
}


void Mapping::execute(TeleWindow *teleWindow)
{
    switch (_type)
    {
        case Shell:
        {
            static const int MAX_ARGS = 10;
            char *args[MAX_ARGS + 2];
            memset(args, 0, sizeof(args));


            int cmdlen = strcspn(_action, " ");
            args[0] = strndup(_action, cmdlen);

            char *arg = _action + cmdlen;
            arg += strspn(arg, " ");
            for (int i = 1; i <= MAX_ARGS; ++i)
            {
                if (*arg == 0)
                    break;

                int arglen = strcspn(arg, " ");
                args[i] = strndup(arg, arglen);

                arg += arglen;
                arg += strspn(arg, " ");
            }

            if (vfork() == 0)
            {
                execvp(args[0], args);
            }
            else
            {
                for (int i = 0; i <= MAX_ARGS; ++i)
                    free(args[i]);

                teleWindow->hide();
            }
            break;
        }

        case Internal:
        {
            teleWindow->internalCommand(_action);
            break;
        }
    }
}
