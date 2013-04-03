//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: XTools.h 140 2010-08-04 12:56:07Z mitrandir $

// XTools - various utility functions for X11

#ifndef __TELESCOPE__XTOOLS_H
#define __TELESCOPE__XTOOLS_H

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#include "LinkedList.h"

class XTools
{
    private:
        static Display *_dpy;

        static int _damage_event_base;
        static int _damage_error_base;

        static int _xrender_event_base;
        static int _xrender_error_base;

        typedef int (*ErrorHandler)(Display *display, XErrorEvent *event);
        static ErrorHandler _prevErrorHandler;

    public:
        static Atom _NET_CLIENT_LIST;
        static Atom _NET_WM_WINDOW_TYPE;
        static Atom _NET_WM_WINDOW_TYPE_NORMAL;
        static Atom _NET_WM_WINDOW_TYPE_SPLASH;
        static Atom _NET_WM_WINDOW_TYPE_DESKTOP;
        static Atom _NET_WM_WINDOW_TYPE_POPUP_MENU;
        static Atom _NET_WM_NAME;
        static Atom _NET_WM_STATE;
        static Atom _NET_WM_STATE_FULLSCREEN;
        static Atom _NET_WM_STATE_HIDDEN;
        static Atom _NET_ACTIVE_WINDOW;
        static Atom _NET_SHOWING_DESKTOP;
        static Atom _NET_CLOSE_WINDOW;
        static Atom WM_STATE;
        static Atom WM_NAME;
        static Atom _NET_WM_ICON;

        static void init(Display *dpy);

        static Window rootWindow();

        static LinkedList<Window> windowList(Window rootWindow, bool includeDesktop = false);

        static char* windowTitle_alloc(Window window);
        static char* windowClass_alloc(Window window);

        static bool checkXRenderExtension();
        static bool checkCompositeExtension();
        static bool checkDamageExtension();


        static void enableCompositeRedirect();
        static void disableCompositeRedirect();


        static int damageEventBase();
        static int damageErrorBase();


        static void switchToWindow(Window window);
        static void closeWindow(Window window);

        static int errorHandler(Display *display, XErrorEvent *event);

        static void showDesktop(bool iconifyAll = false);

        static bool checkIfWindowMinimized(Window window);

        static Window activeWindow();

        static void minimize(Window window);


        static XRenderPictFormat* xrenderFormat();
        static XRenderPictFormat* xrenderRGBAFormat();
        static const XVisualInfo* rgbaVisual();

        static bool fetchWindowIconFromProperty(
            Window window,
            int *width, int *height,
            Pixmap *pixmap,
            Picture *picture
        );

        static Atom windowType(Window window);
};

#endif
