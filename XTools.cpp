//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: XTools.cpp 140 2010-08-04 12:56:07Z mitrandir $

#include "XTools.h"

#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xdamage.h>

#include <Imlib2.h>


Display* XTools::_dpy = 0;

int XTools::_damage_event_base = 0;
int XTools::_damage_error_base = 0;

int XTools::_xrender_event_base = 0;
int XTools::_xrender_error_base = 0;

XTools::ErrorHandler XTools::_prevErrorHandler;

Atom XTools::_NET_CLIENT_LIST;
Atom XTools::_NET_WM_WINDOW_TYPE;
Atom XTools::_NET_WM_WINDOW_TYPE_NORMAL;
Atom XTools::_NET_WM_WINDOW_TYPE_SPLASH;
Atom XTools::_NET_WM_WINDOW_TYPE_DESKTOP;
Atom XTools::_NET_WM_WINDOW_TYPE_POPUP_MENU;
Atom XTools::_NET_WM_NAME;
Atom XTools::_NET_WM_STATE;
Atom XTools::_NET_WM_STATE_FULLSCREEN;
Atom XTools::_NET_WM_STATE_HIDDEN;
Atom XTools::_NET_ACTIVE_WINDOW;
Atom XTools::_NET_SHOWING_DESKTOP;
Atom XTools::_NET_CLOSE_WINDOW;
Atom XTools::WM_STATE;
Atom XTools::WM_NAME;
Atom XTools::_NET_WM_ICON;


#define INIT_ATOM(dpy, ATOMNAME)        ATOMNAME = XInternAtom((dpy), #ATOMNAME, 0L)

void XTools::init(Display *dpy)
{
    _dpy = dpy;

    INIT_ATOM(dpy, _NET_CLIENT_LIST);
    INIT_ATOM(dpy, _NET_WM_WINDOW_TYPE);
    INIT_ATOM(dpy, _NET_WM_WINDOW_TYPE_NORMAL);
    INIT_ATOM(dpy, _NET_WM_WINDOW_TYPE_SPLASH);
    INIT_ATOM(dpy, _NET_WM_WINDOW_TYPE_DESKTOP);
    INIT_ATOM(dpy, _NET_WM_WINDOW_TYPE_POPUP_MENU);
    INIT_ATOM(dpy, _NET_WM_NAME);
    INIT_ATOM(dpy, _NET_WM_STATE);
    INIT_ATOM(dpy, _NET_WM_STATE_FULLSCREEN);
    INIT_ATOM(dpy, _NET_WM_STATE_HIDDEN);
    INIT_ATOM(dpy, _NET_ACTIVE_WINDOW);
    INIT_ATOM(dpy, _NET_SHOWING_DESKTOP);
    INIT_ATOM(dpy, _NET_CLOSE_WINDOW);
    INIT_ATOM(dpy, WM_STATE);
    INIT_ATOM(dpy, WM_NAME);
    INIT_ATOM(dpy, _NET_WM_ICON);

    _prevErrorHandler = XSetErrorHandler(errorHandler);
}

Window XTools::rootWindow()
{
    return RootWindow(_dpy, DefaultScreen(_dpy));
}

LinkedList<Window> XTools::windowList(Window rootWindow, bool includeDesktop)
{
    LinkedList<Window> list;

    Atom real_type;
    int real_format;
    unsigned long items_read, items_left;
    Window *windows;
    if (XGetWindowProperty(_dpy, rootWindow, _NET_CLIENT_LIST, 0L, 8192L, False,
        XA_WINDOW, &real_type, &real_format, &items_read, &items_left, (unsigned char**)&windows)
        != Success)
    {
        return list;
    }
    else
    {
        for (unsigned int i = 0; i < items_read; i++)
        {
            unsigned long items_read, items_left;
            Atom real_type;
            int real_format;
            Atom *windowType;

            if (XGetWindowProperty(_dpy, windows[i], _NET_WM_WINDOW_TYPE, 0L, 1L, False,
                XA_ATOM, &real_type, &real_format, &items_read, &items_left,
                (unsigned char**)&windowType) != Success)
            {
                continue;
            }
            else
            {
                //bool ok = (items_read > 0 && *windowType == _NET_WM_WINDOW_TYPE_NORMAL);
                bool ok = true;
                if (items_read > 0 && *windowType != _NET_WM_WINDOW_TYPE_NORMAL)
                {
                    if ((! includeDesktop) || *windowType != _NET_WM_WINDOW_TYPE_DESKTOP)
                        ok = false;
                }

                XFree((unsigned char*)windowType);

                if (! ok)
                {
                    continue;
                }
            }

            list.append(windows[i]);
        }

        XFree((unsigned char*)windows);

        return list;
    }
}


char* XTools::windowTitle_alloc(Window window)
{
    XTextProperty wmName;
    XGetTextProperty(_dpy, window, &wmName, _NET_WM_NAME);
    if (wmName.value)
    {
        char *ret = strdup((char*)wmName.value);
        XFree(wmName.value);
        return ret;
    }
    else
    {
        XGetTextProperty(_dpy, window, &wmName, WM_NAME);
        if (wmName.value)
        {
            char *ret = strdup((char*)wmName.value);
            XFree(wmName.value);
            return ret;
        }
        else
            return strdup("");
    }
}

char* XTools::windowClass_alloc(Window window)
{
    XClassHint classHint;

    if (XGetClassHint(_dpy, window, &classHint) != 0)
    {
        char *ret = strdup(classHint.res_name);
        XFree(classHint.res_name);
        XFree(classHint.res_class);
        return ret;
    }
    else
        return strdup("");
}



bool XTools::checkCompositeExtension()
{
    int event_base, error_base;
    if (XCompositeQueryExtension(_dpy, &event_base, &error_base))
    {
        int major = 0, minor = 2;
        XCompositeQueryVersion(_dpy, &major, &minor);

        if (major > 0 || minor >= 2)
            return true;
        else
            return false;
    }
    else
        return false;
}


bool XTools::checkDamageExtension()
{
    if (! XDamageQueryExtension(_dpy, &_damage_event_base, &_damage_error_base))
        return false;

    return true;
}


bool XTools::checkXRenderExtension()
{
    if (! XRenderQueryExtension(_dpy, &_xrender_event_base, &_xrender_error_base))
        return false;

    return true;
}


void XTools::enableCompositeRedirect()
{
    for (int i = 0; i < ScreenCount(_dpy); i++)
        XCompositeRedirectSubwindows(_dpy, RootWindow(_dpy, i), CompositeRedirectAutomatic);
}

void XTools::disableCompositeRedirect()
{
    for (int i = 0; i < ScreenCount(_dpy); i++)
        XCompositeUnredirectSubwindows(_dpy, RootWindow(_dpy, i), CompositeRedirectAutomatic);
}


int XTools::damageEventBase()
{
    return _damage_event_base;
}

int XTools::damageErrorBase()
{
    return _damage_error_base;
}



void XTools::switchToWindow(Window window)
{
    XEvent event;

    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = _NET_ACTIVE_WINDOW;
    event.xclient.window = window;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 0;
    event.xclient.data.l[1] = 0;
    event.xclient.data.l[2] = 0;
    event.xclient.data.l[3] = 0;
    event.xclient.data.l[4] = 0;

    XSendEvent(_dpy, DefaultRootWindow(_dpy), False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

void XTools::closeWindow(Window window)
{
    XEvent event;

    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = _NET_CLOSE_WINDOW;
    event.xclient.window = window;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 0;
    event.xclient.data.l[1] = 0;
    event.xclient.data.l[2] = 0;
    event.xclient.data.l[3] = 0;
    event.xclient.data.l[4] = 0;

    XSendEvent(_dpy, DefaultRootWindow(_dpy), False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
}


int XTools::errorHandler(Display *display, XErrorEvent *event)
{
    printf("X Error! [%d, %d, %d]\n", event->error_code, event->request_code, event->minor_code);

//    _prevErrorHandler(display, event);

    return 0;
}


void XTools::showDesktop(bool iconifyAll)
{
    if (! iconifyAll)
    {
        // Right way if WM supports _NET_SHOWING_DESKTOP

        XEvent event;

        Window root = DefaultRootWindow(_dpy);

        event.xclient.type = ClientMessage;
        event.xclient.serial = 0;
        event.xclient.send_event = True;
        event.xclient.message_type = _NET_SHOWING_DESKTOP;
        event.xclient.window = root;
        event.xclient.format = 32;
        event.xclient.data.l[0] = 1;
        event.xclient.data.l[1] = 1;
        event.xclient.data.l[2] = 0;
        event.xclient.data.l[3] = 0;
        event.xclient.data.l[4] = 0;

        XSendEvent(_dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
    }
    else
    {
        // Bad and ugly way -- minimizing all windows

        LinkedList<Window> windows = windowList(RootWindow(_dpy, DefaultScreen(_dpy)));
        for (LinkedList<Window>::Iter i = windows.head(); i; ++i)
            if (! checkIfWindowMinimized(*i))
                minimize(*i);
    }
}



bool XTools::checkIfWindowMinimized(Window window)
{
    unsigned long *property = NULL;
    unsigned long nitems;
    unsigned long left;
    Atom actual_type;
    int actual_format;

    int status = XGetWindowProperty(_dpy, window, WM_STATE,
        0, 1,
        False, WM_STATE,
        &actual_type, &actual_format,
        &nitems, &left,
        (unsigned char**)&property);

    if (status != Success)
        return false;

    bool minimized = property ? (*property == IconicState) : false;

    XFree(property);

    return minimized;
}



Window XTools::activeWindow()
{
    unsigned long nitems;
    unsigned long left;
    Atom actual_type;
    int actual_format;

    Window *property;

    int status = XGetWindowProperty(_dpy, DefaultRootWindow(_dpy), _NET_ACTIVE_WINDOW,
        0, 1,
        False, XA_WINDOW,
        &actual_type, &actual_format,
        &nitems, &left,
        (unsigned char**)&property
    );

    if (status != Success)
        return 0;

    Window result = *property;

    XFree(property);

    return result;
}


Atom XTools::windowType(Window window)
{
    unsigned long nitems;
    unsigned long left;
    Atom actual_type;
    int actual_format;

    Atom *property;

    int status = XGetWindowProperty(_dpy, window, _NET_WM_WINDOW_TYPE,
        0, 1,
        False, XA_ATOM,
        &actual_type, &actual_format,
        &nitems, &left,
        (unsigned char**)&property
    );

    if (status != Success)
        return 0;

    Atom result = *property;

    XFree(property);

    return result;
}


void XTools::minimize(Window window)
{
    XIconifyWindow(_dpy, window, DefaultScreen(_dpy));
}



XRenderPictFormat* XTools::xrenderFormat()
{
    static XRenderPictFormat *format = 0;

    if (format == 0)
    {
        Visual *visual = DefaultVisual(_dpy, DefaultScreen(_dpy));
        format = XRenderFindVisualFormat(_dpy, visual);
    }

    return format;
}


const XVisualInfo* XTools::rgbaVisual()
{
    static bool inited = false;
    static XVisualInfo rgbaVisual;

    if (inited == false)
    {
        int scr = DefaultScreen(_dpy);
        if (XMatchVisualInfo(_dpy, scr, 32, TrueColor, &rgbaVisual) == 0)
            fprintf(stderr, "Cannot find rgba visual\n");
    }

    return &rgbaVisual;
}


XRenderPictFormat* XTools::xrenderRGBAFormat()
{
    static XRenderPictFormat *rgbaFormat = 0;

    if (rgbaFormat == 0)
    {
        rgbaFormat = XRenderFindVisualFormat(_dpy, rgbaVisual()->visual);
    }

    return rgbaFormat;
}



bool XTools::fetchWindowIconFromProperty(
        Window window,
        int *width, int *height,
        Pixmap *pixmap,
        Picture *picture
    )
{
    if (width) *width = 0;
    if (height) *height = 0;
    if (pixmap) *pixmap = 0;
    if (picture) *picture = 0;

    Atom real_type;
    int real_format;
    unsigned long items_read, items_left;

    unsigned int *iconData;

    if (XGetWindowProperty(_dpy, window, _NET_WM_ICON, 0, 8192L, False,
            XA_CARDINAL, &real_type, &real_format, &items_read, &items_left,
            (unsigned char **)&iconData) == Success && items_read > 0)
    {
        int w = iconData[0];
        int h = iconData[1];

        if (width) *width = w;
        if (height) *height = h;

        if (pixmap)
        {
            Imlib_Image icon = imlib_create_image_using_data( w, h, &iconData[2]);
            imlib_context_set_image(icon);
            imlib_image_set_has_alpha(1);

            *pixmap = XCreatePixmap(_dpy, RootWindow(_dpy, DefaultScreen(_dpy)), w, h, 32);

            imlib_context_set_display(_dpy);
            imlib_context_set_visual(rgbaVisual()->visual);
            imlib_context_set_colormap(DefaultColormap(_dpy, DefaultScreen(_dpy)));
            imlib_context_set_drawable(*pixmap);

            // Making image with premultiplied alpha
            Imlib_Image premul = imlib_create_image(w, h);
            imlib_context_set_image(premul);
            imlib_image_set_has_alpha(1);
            imlib_context_set_color(0, 0, 0, 255);
            imlib_context_set_blend(0);
            imlib_image_fill_rectangle(0, 0, w, h);
            imlib_context_set_blend(1);
            imlib_blend_image_onto_image(icon, 0, 0, 0, w, h, 0, 0, w, h);
            imlib_image_copy_alpha_to_image(icon, 0, 0);

            imlib_context_set_blend(0);
            imlib_render_image_on_drawable(0, 0);

            imlib_free_image();
            imlib_context_set_image(icon);
            imlib_free_image();

            if (picture)
            {
                *picture = XRenderCreatePicture(_dpy, *pixmap,
                    xrenderRGBAFormat(), 0, 0
                );
            }
        }

        XFree((unsigned char*)iconData);

        return true;
    }

    return false;
}
