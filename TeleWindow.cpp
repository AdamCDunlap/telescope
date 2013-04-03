//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: TeleWindow.cpp 184 2011-06-28 19:51:40Z mitrandir $

#include "TeleWindow.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <sys/time.h>

#include <X11/Xatom.h>

#include "XTools.h"
#include "Thumbnail.h"
#include "Settings.h"
#include "Resources.h"

#include "Image.h"

#include "XEventLoop.h"

#ifdef LAUNCHER
    #include "LauncherWindow.h"
#endif


TeleWindow::TeleWindow(Display *dpy)
    :_dpy(dpy), _mappings(dpy)
{
    int scr = DefaultScreen(_dpy);
    _rootWindow = RootWindow(_dpy, scr);
    int depth = DefaultDepth(_dpy, scr);
    Visual *visual = DefaultVisual(_dpy, scr);
    Colormap colormap = DefaultColormap(_dpy, scr);

    _width = DisplayWidth(_dpy, scr);
    _height = DisplayHeight(_dpy, scr);


    // Initializing imlib2

    imlib_context_set_display(_dpy);
    imlib_context_set_visual(visual);
    imlib_context_set_colormap(colormap);


    // Initialization of scrolling

    _scrollBaseX = 0;
    _scrollBaseY = 0;
    _scrollX = 0;
    _scrollY = 0;
    _buttonPressed = false;




    // Creating main window

    XSetWindowAttributes createAttrs;
    createAttrs.save_under = True; // This seems to not work, but let's keep it

    _win = XCreateWindow(_dpy, _rootWindow, 0, 0, _width, _height,
        0, depth, InputOutput, visual,
        CWSaveUnder,
        &createAttrs);

    _gc = XCreateGC(_dpy, _win, 0, 0);
    XSetGraphicsExposures(_dpy, _gc, false);

    XSelectInput(_dpy, _win,
        ExposureMask        |
        ButtonPressMask     |
        ButtonReleaseMask   |
        ButtonMotionMask    |
        StructureNotifyMask |
        KeyPressMask        |
        KeyReleaseMask
    );


    // Setting Window type as Splash. This tells window manager
    // to not try to decorate our window.
    Atom windowType = XTools::_NET_WM_WINDOW_TYPE_SPLASH;
    XChangeProperty(_dpy, _win, XTools::_NET_WM_WINDOW_TYPE,
        XA_ATOM, 32, PropModeReplace,
        (const unsigned char*)&windowType, 1);



    // Initializing Xft

    _xftFont = XftFontOpen(_dpy, scr,
        XFT_FAMILY, XftTypeString, "sans",
        XFT_PIXEL_SIZE, XftTypeInteger, Settings::instance()->fontSize(),
        NULL);


    // Double buffering pixmap
    _buffer = 0;
    recreateBuffer();



    _shown = false;




    XSelectInput(_dpy, _rootWindow, StructureNotifyMask | PropertyChangeMask | KeyPressMask | KeyReleaseMask);


    // Hotkey keycode
    KeySym keysym = XStringToKeysym(Settings::instance()->hotKey());
    _hotKeyCode = XKeysymToKeycode(_dpy, keysym);
    _hotKeyPressed = false;
    XGrabKey(_dpy, _hotKeyCode,        0, _rootWindow, False, GrabModeAsync, GrabModeAsync);
    XGrabKey(_dpy, _hotKeyCode, Mod2Mask, _rootWindow, False, GrabModeAsync, GrabModeAsync);

    _activeThumbnail = 0;


    markThumbnailsListDirty();


    XEventLoop::instance()->addHandler(this);
    XEventLoop::instance()->addIdleTask(this);
}

TeleWindow::~TeleWindow()
{
     XUngrabKey(_dpy, _hotKeyCode, AnyModifier, _rootWindow);


    for (LinkedList<Thumbnail*>::Iter i = _thumbnails.head(); i; ++i)
        delete *i;


    delete _buffer;

    XftFontClose(_dpy, _xftFont);

    XFreeGC(_dpy, _gc);
    XDestroyWindow(_dpy, _win);
}


Display* TeleWindow::display()
{
    return _dpy;
}


Window TeleWindow::window()
{
    return _win;
}


bool TeleWindow::show()
{
    updateThumbnailsList();

    if (_thumbnails.size() == 0)
        return false;

    _shown = true;

    Window activeWindow = XTools::activeWindow();

    Thumbnail *prevActiveThumbnail = _activeThumbnail;
    _activeThumbnail = 0;
    for (LinkedList<Thumbnail*>::Iter i = _thumbnails.head(); i; ++i)
        if ((*i)->clientWindow() == activeWindow)
            _activeThumbnail = *i;

    if (_activeThumbnail != prevActiveThumbnail)
    {
        if (_activeThumbnail) _activeThumbnail->redraw();
        if (prevActiveThumbnail) prevActiveThumbnail->redraw();
    }


    XMapWindow(_dpy, _win);
    XTools::switchToWindow(_win);

#ifdef MAEMO4
    // Hack for Nokia's builtin mediaplayer that leaves it's
    // video overlay on screen after task switching when
    // Composite is enabled
    if (_activeThumbnail)
    {
        if (_activeThumbnail->mustBeIconifiedBeforeTelescope())
        {
            paint();
            XSync(_dpy, 0);
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;
            select(0, 0, 0, 0, &timeout);
            _activeThumbnail->minimize();
        }
    }
#endif



#ifdef LAUNCHER

    if (LauncherWindow::instance() && LauncherWindow::instance()->shown())
    {
        paint();
        XFlush(_dpy);
        LauncherWindow::instance()->hide();
    }

#endif


    return _shown;
}

void TeleWindow::hide()
{
    XUnmapWindow(_dpy, _win);

    _shown = false;
}

bool TeleWindow::shown()
{
    return _shown;
}



void TeleWindow::markThumbnailsListDirty()
{
    _thumbnailsListDirty = true;

    if (_shown)
    {
        updateThumbnailsList();

        if (_thumbnails.size() == 0)
            hide();
    }
}



void TeleWindow::updateThumbnailsList()
{
    if (_thumbnailsListDirty == false)
        return;



    bool wasChanged = false;


    LinkedList<Window> windows = XTools::windowList(_rootWindow,
        Settings::instance()->showDesktopThumbnail());
    for (LinkedList<Window>::Iter i = windows.head(); i; ++i)
        if (*i != _win)
        {
            bool found = false;
            for (LinkedList<Thumbnail*>::Iter j = _thumbnails.head(); j; ++j)
                if ((*j)->clientWindow() == *i)
                {
                    found = true;
                    break;
                }

            if (! found)
            {
                Thumbnail *th = new Thumbnail(this, *i);
                _thumbnails.append(th);
                wasChanged = true;
            }
        }

    LinkedList<Thumbnail*> thumbsToDelete;
    for (LinkedList<Thumbnail*>::Iter i = _thumbnails.head(); i; ++i)
        if (! windows.contains((*i)->clientWindow()))
            thumbsToDelete.append(*i);

    for (LinkedList<Thumbnail*>::Iter i = thumbsToDelete.head(); i; ++i)
    {
        removeThumbnail(*i);
        delete *i;
        wasChanged = true;
    }


    if (wasChanged)
        layoutThumbnails();

    _thumbnailsListDirty = false;
}


inline int min(int a, int b)
{
    return a < b ? a : b;
}


void TeleWindow::layoutThumbnails()
{
    int n = _thumbnails.size();

    if (n == 0)
    {
        paint();
        return;
    }


    int maxSize = 0;
    int maxColumns = 0;

    for (int columns = n; columns >= 1; --columns)
    {
        int rows = (n + columns - 1) / columns;

        int border = (int)(_width / columns * 0.1f);
        int width = _width / columns - border;
        int height = _height / rows - border;

        int resultSize = height * _width / _height;
        if (width < resultSize)
            resultSize = width;

        if (resultSize > maxSize)
        {
            maxSize = resultSize;
            maxColumns = columns;
        }
    }

    int columns = maxColumns;
    int rows = (n + columns - 1) / columns;

    int tileWidth = _width / columns;
    int tileHeight = _height / rows;
    int border = (int)(_width / columns * 0.1f);

    int thumbWidth, thumbHeight;

    if (n > 1)
    {
        thumbWidth = _width / columns - border;
        thumbHeight = _height / rows - border;
    }
    else
    {
        thumbWidth = _width * 2 / 3;
        thumbHeight = _height * 2 / 3;
    }

    int tileXOffset = (tileWidth - thumbWidth) / 2;
    int tileYOffset = (tileHeight-thumbHeight) / 2;

    int lastRowX = 0;

    int index = 0;
    LinkedList<Thumbnail*>::Iter thumb = _thumbnails.head();
    for (int row = 0; row < rows; ++row)
    {
        if (row == rows - 1)
        {
            lastRowX = (_width - (n - index) * tileWidth) / 2;
        }

        int tileY = row * tileHeight;

        for (int column = 0; column < columns; ++column, ++thumb, ++index)
        {
            if (thumb == 0)
                break;

            int tileX = lastRowX + column * tileWidth;

            int x = tileX + tileXOffset;
            int y = tileY + tileYOffset;

            (*thumb)->fitIn(x, y, thumbWidth, thumbHeight);
        }
    }

    if (_shown)
        paint();
}


void TeleWindow::removeThumbnail(Thumbnail *thumb)
{
    _thumbnails.removeByValue(thumb);
    if (_activeThumbnail == thumb)
        _activeThumbnail = 0;
}


void TeleWindow::onEvent(XEvent *event)
{
    if (event->xany.window == _rootWindow)
    {
        onRootEvent(event);
        return;
    }

    if (event->xany.window == _win)
    {
        onTeleWindowEvent(event);
        return;
    }
    else
    {
        bool found = false;
        for (LinkedList<Thumbnail*>::Iter i = _thumbnails.head(); i; ++i)
        {
            if ((*i)->clientWindow() == event->xany.window)
            {
                if (event->type == DestroyNotify)
                {
                    (*i)->setClientDestroyed(true);
                    removeThumbnail(*i);
                    delete *i;
                    // markThumbnailsListDirty();
                    layoutThumbnails();
                }
                else
                    (*i)->onClientEvent(event);

                found = true;
                break;
            }
        }
        if (found)
            return;
    }
}



void TeleWindow::onRootEvent(XEvent *event)
{
    if (event->type == KeyPress)
    {
        if (event->xkey.keycode == _hotKeyCode)
            onHotKeyPress();
        else
            _mappings.handleEvent(this, Mapping::GlobalPress, event->xkey.keycode);
    }
    else if (event->type == KeyRelease)
    {
        if (event->xkey.keycode == _hotKeyCode)
            onHotKeyRelease();
    }
//    else if (event->type == MapNotify)
//    {
//    }
    else if (event->type == PropertyNotify)
    {
//        char *name = XGetAtomName(_dpy, event->xproperty.atom);
//        printf("Root PropertyNotify: %s\n", name);
//        XFree(name);

        if (_shown && event->xproperty.atom == XTools::_NET_ACTIVE_WINDOW)
        {
            Window activeWindow = XTools::activeWindow();
            if (activeWindow != _win)
//                if (XTools::windowType(activeWindow) != XTools::_NET_WM_WINDOW_TYPE_POPUP_MENU)
                    hide();
        }

        if (event->xproperty.atom == XTools::_NET_CLIENT_LIST)
        {
            // FIXME: This notify seems to be sent too often,
            // even when no windows actually changed
            markThumbnailsListDirty();
        }
    }
    else if (event->type == ConfigureNotify && event->xconfigure.window == _rootWindow)
    {
        if (event->xconfigure.width != _width || event->xconfigure.height != _height)
        {
            setNewSize(event->xconfigure.width, event->xconfigure.height);
        }
    }
//    else
//        printf("Unknown root event: %d\n", event->type);
}



void TeleWindow::onTeleWindowEvent(XEvent *event)
{
    if (event->type == ButtonPress)
        onButtonPress(&event->xbutton);
    else if (event->type == ButtonRelease)
        onButtonRelease(&event->xbutton);
    else if (event->type == MotionNotify)
        onButtonMotion(&event->xmotion);
    else if (event->type == Expose && event->xexpose.count < 1)
        //paint();
        _repaintOnIdle = true;
    else if (event->type == ConfigureNotify)
    {
        bool relayout = _width != event->xconfigure.width || _height != event->xconfigure.height;
        _width = event->xconfigure.width;
        _height= event->xconfigure.height;
        if (relayout)
            layoutThumbnails();
    }
    else if (event->type == KeyPress)
    {
        _mappings.handleEvent(this, Mapping::Press, event->xkey.keycode);
    }
}


void TeleWindow::paint()
{
    if (! _shown)
        return;


    XCopyArea(_dpy, Resources::instance()->wallpaper()->pixmap(), _buffer->pixmap(), _gc,
        0, 0, _width, _height, 0, 0
    );


    for (LinkedList<Thumbnail*>::Iter i = _thumbnails.head(); i; ++i)
    {
        (*i)->drawPreview();
        blitThumb(*i);
    }


    blitBuffer();
}

void TeleWindow::blitThumb(Thumbnail *thumb)
{
    XRenderComposite(_dpy, PictOpOver,
        thumb->image()->picture(), None, _buffer->picture(),
        0, 0,
        0, 0,
        thumb->x(), thumb->y(),
        thumb->width(), thumb->height()
    );
}


void TeleWindow::blitBuffer()
{
    XCopyArea(_dpy, _buffer->pixmap(), _win, _gc,
        0, 0, _width, _height, 0, 0
    );
}


void TeleWindow::onThumbRedrawed(Thumbnail *thumb)
{
    blitThumb(thumb);
    blitBuffer();
}



void TeleWindow::onHotKeyPress()
{
    if (_hotKeyPressed)
    { // Long press
        if (_shown)
            hide();
        showDesktop();
        return;
    }

    _hotKeyPressed = true;

    if (! _shown)
    {
        if (! show())
        {
            #ifdef LAUNCHER
                if (! Settings::instance()->disableLauncher())
                    LauncherWindow::instance()->show();
            #endif
        }
    }
    else
    {
        if (Settings::instance()->disableLauncher())
            _mappings.handleEvent(this, Mapping::Press, _hotKeyCode);
        else
        {
            #ifdef LAUNCHER
                LauncherWindow::instance()->show();
                XTools::switchToWindow(LauncherWindow::instance()->window());
                LauncherWindow::instance()->paint();
                XFlush(_dpy);
                hide();
            #endif
        }
    }
}

void TeleWindow::onHotKeyRelease()
{
    char keysPressed[32];
    XQueryKeymap(_dpy, keysPressed);
    if ( (keysPressed[_hotKeyCode >> 3] >> (_hotKeyCode & 0x07)) & 0x01)
        return;

    _hotKeyPressed = false;
}



void TeleWindow::onButtonPress(XButtonEvent *event)
{
    _wasScrolling = false;
    _buttonPressed = true;
    _buttonPressX = event->x;
    _buttonPressY = event->y;
    _scrollBaseX = _scrollX;
    _scrollBaseY = _scrollY;
}

void TeleWindow::onButtonRelease(XButtonEvent *event)
{
    _buttonPressed = false;

    if (! _wasScrolling)
    {
        bool missed = true;

        for (LinkedList<Thumbnail*>::Iter i = _thumbnails.head(); i; ++i)
            if ((*i)->handleMousePress(event->x, event->y))
            {
                missed = false;
                break;
            }

        if (missed)
        {
            hide();
            showDesktop();
        }
    }
}


void TeleWindow::onButtonMotion(XMotionEvent *event)
{
    if (_buttonPressed && Settings::instance()->scrollingEnabled())
    {
        _scrollX = _scrollBaseX + event->x - _buttonPressX;
        _scrollY = _scrollBaseY + event->y - _buttonPressY;

        if (! _wasScrolling)
        {
            if (abs(event->x - _buttonPressX) > 20 ||
                abs(event->y - _buttonPressY) > 20)
                _wasScrolling = true;
        }


//        paint();
        _repaintOnIdle = true;
    }
}


void TeleWindow::onIdle()
{
    if (_repaintOnIdle)
    {
        paint();
        _repaintOnIdle = false;
    }
}


void TeleWindow::internalCommand(const char *action)
{
    if (strcmp(action, "switchToSelected") == 0)
    {
        if (! Settings::instance()->disableSelection())
        {
            hide();
            if (_activeThumbnail != 0)
                _activeThumbnail->switchToClient();
            else
                showDesktop();
        }
    }
    else if (strcmp(action, "selectNext") == 0)
    {
        if (! Settings::instance()->disableSelection())
        {
            Thumbnail *prevActiveThumbnail = _activeThumbnail;
            if (_activeThumbnail == 0)
            {
                if (_thumbnails.size() > 0)
                    _activeThumbnail = *_thumbnails.head();
            }
            else
            {
                int index = 0;
                bool changed = false;
                for (LinkedList<Thumbnail*>::Iter i = _thumbnails.head(); i; ++i, ++index)
                    if ((*i) == _activeThumbnail)
                    {
                        if (index < _thumbnails.size() - 1)
                            _activeThumbnail = *(++i);
                        else
                            _activeThumbnail = *_thumbnails.head();
                        changed = true;
                        break;
                    }

                if (! changed)
                    _activeThumbnail = *_thumbnails.head();
            }

            if (_activeThumbnail) _activeThumbnail->redraw();
            if (prevActiveThumbnail) prevActiveThumbnail->redraw();

            paint();
        }
    }
    else if (strcmp(action, "selectPrev") == 0)
    {
        if (! Settings::instance()->disableSelection())
        {
            Thumbnail *prevActiveThumbnail = _activeThumbnail;
            if (_activeThumbnail == 0)
            {
                if (_thumbnails.size() > 0)
                    _activeThumbnail = *_thumbnails.tail();
            }
            else
            {
                int index = 0;
                bool changed = false;
                for (LinkedList<Thumbnail*>::Iter i = _thumbnails.head(); i; ++i, ++index)
                    if ((*i) == _activeThumbnail)
                    {
                        if (index > 0)
                            _activeThumbnail = *(--i);
                        else
                            _activeThumbnail = *_thumbnails.tail();
                        changed = true;
                        break;
                    }

                if (! changed)
                    _activeThumbnail = *_thumbnails.tail();
            }

            if (_activeThumbnail) _activeThumbnail->redraw();
            if (prevActiveThumbnail) prevActiveThumbnail->redraw();

            paint();
        }
    }
    else if (   strcmp(action, "selectRight") == 0
             || strcmp(action, "selectLeft") == 0
             || strcmp(action, "selectDown") == 0
             || strcmp(action, "selectUp") == 0
            )
    {
        if (_activeThumbnail != 0 && ! Settings::instance()->disableSelection())
        {
            Thumbnail *prevActiveThumbnail = _activeThumbnail;

            Thumbnail *newThumbnail = 0;
            if (strcmp(action, "selectRight") == 0)
                newThumbnail = findThumbnailByCoords(_activeThumbnail, 1);
            if (strcmp(action, "selectLeft") == 0)                      
                newThumbnail = findThumbnailByCoords(_activeThumbnail, 3);
            if (strcmp(action, "selectUp") == 0)                        
                newThumbnail = findThumbnailByCoords(_activeThumbnail, 0);
            if (strcmp(action, "selectDown") == 0)                      
                newThumbnail = findThumbnailByCoords(_activeThumbnail, 2);

            if (newThumbnail)
                _activeThumbnail = newThumbnail;

            if (_activeThumbnail) _activeThumbnail->redraw();
            if (prevActiveThumbnail) prevActiveThumbnail->redraw();

            paint();
        }
    }
    else
    {
        fprintf(stderr, "Unknown internal command: %s\n", action);
    }
}


Thumbnail* TeleWindow::findThumbnailByCoords(
    Thumbnail *orig,
    int direction
)
{
    Thumbnail *foundThumb = 0;
    int max = INT_MAX;

    for (LinkedList<Thumbnail*>::Iter i = _thumbnails.head(); i; ++i)
    {
        Thumbnail *thumb = *i;

        if (thumb == orig)
            continue;

        int rx = thumb->x() - orig->x();
        int ry = thumb->y() - orig->y();
        int arx = abs(rx);
        int ary = abs(ry);

        if (ry < 0 && ary > arx && direction != 0) continue;
        if (rx > 0 && arx > ary && direction != 1) continue;
        if (ry > 0 && ary > arx && direction != 2) continue;
        if (rx < 0 && arx > ary && direction != 3) continue;

        if (arx + ary < max)
        {
            foundThumb = thumb;
            max = arx + ary;
        }
    }

    return foundThumb;
}



void TeleWindow::setNewSize(int width, int height)
{
    _width = width;
    _height = height;

    XMoveResizeWindow(_dpy, _win, 0, 0, _width, _height);

    recreateBuffer();

    Resources::instance()->reloadWallpaper();

    layoutThumbnails();
}



void TeleWindow::recreateBuffer()
{
    if (_buffer)
        delete _buffer;

    _buffer = new Image(_dpy, _width, _height, DefaultDepth(_dpy, DefaultScreen(_dpy)));
}


/*
void TeleWindow::animate(Thumbnail *thumb, bool toSmall)
{
        XFlush(_dpy);


        XWindowAttributes attrs;
        XGetWindowAttributes(_dpy, thumb->clientWindow(), &attrs);

        int bigx, bigy;
        int bigw = attrs.width;
        int bigh = attrs.height;
        Window child;
        XTranslateCoordinates(_dpy, thumb->clientWindow(), _rootWindow,
            0, 0, &bigx, &bigy, &child);

        int xoffset = (thumb->width() - thumb->realWidth()) / 2;
        int yoffset = (thumb->height() - thumb->realHeight()) / 2;
        int smallx = thumb->x() + xoffset;
        int smally = thumb->y() + yoffset;
        int smallw = thumb->realWidth();
        int smallh = thumb->realHeight();


        if (! toSmall)
        {
            int t;
            t = smallx; smallx = bigx; bigx = t;
            t = smally; smally = bigy; bigy = t;
            t = smallw; smallw = bigw; bigw = t;
            t = smallh; smallh = bigh; bigh = t;
        }

        int prevx = 0, prevy = 0, prevw = _width, prevh = _height;


        double step = toSmall ? 0.33 : 0.50;
        for (double a = 0.0; a <= 1.0; a += step)
        {
            int x = (int)(((double)bigx) * (1.0 - a) + ((double)smallx) * a);
            int y = (int)(((double)bigy) * (1.0 - a) + ((double)smally) * a);
            int w = (int)(((double)bigw) * (1.0 - a) + ((double)smallw) * a);
            int h = (int)(((double)bigh) * (1.0 - a) + ((double)smallh) * a);

            double scale = w;
            scale /= attrs.width;

            XTransform xform = {{
                { XDoubleToFixed(1), XDoubleToFixed(0), XDoubleToFixed(0) },
                { XDoubleToFixed(0), XDoubleToFixed(1), XDoubleToFixed(0) },
                { XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(scale) },
            }};

            XRenderSetPictureTransform(_dpy, thumb->clientPicture(), &xform);

            if (toSmall)
                XCopyArea(_dpy, _bufferPix, _win, _gc,
                    prevx, prevy, prevw, prevh, prevx, prevy);

            XRenderComposite(_dpy, PictOpSrc,
                thumb->clientPicture(), None, _picture,
                (int)(- (1.0 - scale) * thumb->clientDecoX()),
                (int)(- (1.0 - scale) * thumb->clientDecoY()),
                0, 0,
                x, y,
                w, h
            );

            prevx = x;
            prevy = y;
            prevw = w;
            prevh = h;

            XSync(_dpy, 0);

            //struct timeval ts;
            //ts.tv_sec = 0;
            //ts.tv_usec = 1000;
            //select(0, 0, 0, 0, &ts);
        }
}
*/



void TeleWindow::showDesktop()
{
    XTools::showDesktop(Settings::instance()->showDesktopByIconify());
}
