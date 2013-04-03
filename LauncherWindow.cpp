/*
 *  Copyright (c) 2010 Andry Gunawan <angun33@gmail.com>
 *
 *  Parts of this file are based on Telescope which is
 *  Copyright (c) 2010 Ilya Skriblovsky <Ilya.Skriblovsky@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <X11/Xatom.h>
#include <X11/keysymdef.h>

#include <dirent.h>

#include "LauncherWindow.h"
#include "Resources.h"
#include "Settings.h"
#include "MenuReader.h"
#include "XTools.h"
#include "constant.h"

#include "Image.h"

#include "XEventLoop.h"
#include "DBus.h"



LauncherWindow* LauncherWindow::_instance = 0;


LauncherWindow::LauncherWindow ( Display *dpy/*, SectionList *list */)
        :_dpy ( dpy )//, _sections ( list )
{
    LauncherWindow::_instance = this;


    MenuReader::getInstance()->processMenu();


    int scr = DefaultScreen ( _dpy );
    _rootWindow = RootWindow ( _dpy, scr );
    int depth = DefaultDepth ( _dpy, scr );
    Visual *visual = DefaultVisual ( _dpy, scr );

    _width = DisplayWidth(_dpy, scr);
    _height = DisplayHeight(_dpy, scr);

//    _prevButtonPressed = false;
//    _nextButtonPressed = false;

    // Creating main window
    XSetWindowAttributes createAttrs;
    createAttrs.save_under = True; // This seems to not work, but let's keep it

    _win = XCreateWindow ( _dpy, _rootWindow, 0, 0, _width, _height,
                           0, depth, InputOutput, visual,
                           CWSaveUnder,
                           &createAttrs );

    _gc = XCreateGC ( _dpy, _win, 0, 0 );
    XSetGraphicsExposures ( _dpy, _gc, false );

    XSelectInput ( _dpy, _win,
                   ExposureMask           |
                   ButtonPressMask        |
                   ButtonReleaseMask      |
                   StructureNotifyMask    |
                   KeyPressMask           |
                   KeyReleaseMask         |
                   ButtonMotionMask
                 );


    // Setting Window type as Splash. This tells window manager
    // to not try to decorate our window.
    Atom windowType = XTools::_NET_WM_WINDOW_TYPE_SPLASH;
    XChangeProperty ( _dpy, _win, XTools::_NET_WM_WINDOW_TYPE,
                      XA_ATOM, 32, PropModeReplace,
                      ( const unsigned char* ) &windowType, 1 );

    XSelectInput ( _dpy, _rootWindow, StructureNotifyMask | SubstructureNotifyMask | PropertyChangeMask );

    // Double buffering pixmap
    _buffer = 0;
    recreateBuffer();
    redrawSections();


    _shown = false;

    _currentSection = 0;

    _repaintOnIdle = false;

    XEventLoop::instance()->addHandler(this);
    XEventLoop::instance()->addIdleTask(this);


    // Initializing sections panel
    _panelBackground = new Image(_dpy, Settings::instance()->panelBackgroundFilename());
    _panelBackground->setRepeatType(RepeatNormal);
    _panelFocusLeft = new Image(_dpy, Settings::instance()->panelFocusLeftFilename());
    _panelFocusRight = new Image(_dpy, Settings::instance()->panelFocusRightFilename());
    _panelFocusMiddle = new Image(_dpy, Settings::instance()->panelFocusMiddleFilename());
    _panelFocusMiddle->setRepeatType(RepeatNormal);

    _panelWidth = _width > _height ? _width : _height;
    _panelHeight = _panelBackground->height();
    _panel = new Image(_dpy, _panelWidth, _panelHeight, DefaultDepth(_dpy, DefaultScreen(_dpy)));


    // Initializing Xft for drawing section titles
    _xftDraw = XftDrawCreate(_dpy, _panel->pixmap(), DefaultVisual(_dpy, DefaultScreen(_dpy)), DefaultColormap(_dpy, DefaultScreen(_dpy)));
    _xftFont = XftFontOpen(_dpy, DefaultScreen(_dpy),
        XFT_FAMILY, XftTypeString, "sans",
        XFT_PIXEL_SIZE, XftTypeInteger, 23,
        XFT_WEIGHT, XftTypeInteger, XFT_WEIGHT_BOLD,
        NULL
    );


    _longtapTimeout = 0;


    // Load category icons
    _categoryIconsBarVisible = false;
    _categoryIconsBar = 0;
    buildCategoryIconsBar();

    _ignoreNextButtonRelease = false;
}

LauncherWindow::~LauncherWindow()
{
    XftFontClose(_dpy, _xftFont);
    XftDrawDestroy(_xftDraw);

    delete _panel;
    delete _panelBackground;
    delete _panelFocusLeft;
    delete _panelFocusRight;
    delete _panelFocusMiddle;



//    delete _sections;

    delete _buffer;

    XFreeGC ( _dpy, _gc );
    XDestroyWindow ( _dpy, _win );
}

Display* LauncherWindow::display()
{
    return _dpy;
}


Window LauncherWindow::window()
{
    return _win;
}


void LauncherWindow::show()
{
    _shown = true;

    XMapWindow ( _dpy, _win );

    XTools::switchToWindow ( _win );
}

void LauncherWindow::hide()
{
    XUnmapWindow ( _dpy, _win );

    _shown = false;
    _categoryIconsBarVisible = false;
}

bool LauncherWindow::shown()
{
    return _shown;
}

void LauncherWindow::quit()
{
    hide();
    _repaintOnIdle = false;
    _breakEventLoop = true;
}

void LauncherWindow::_goPrevious()
{
//    _prevButtonPressed = false;

    // go to previous section
    if ( _currentSection == 0 )
        _currentSection = MenuReader::getInstance()->sectionList()->getSize() - 1;
    else
        _currentSection--;

    if ( _shown )
        _repaintOnIdle = true;
}

void LauncherWindow::_goNext()
{
//    _nextButtonPressed = false;
    // go to next section
    if ( _currentSection == MenuReader::getInstance()->sectionList()->getSize() - 1 )
        _currentSection = 0;
    else
        _currentSection++;

    if ( _shown )
        _repaintOnIdle = true;
}

void LauncherWindow::redrawSections()
{
//    for ( uint i = 0; i < _sections->getSize(); i++ )
//    {
//        printf("drawing section %d\n", i);
//        _sections->get(i)->redraw(_dpy, _width, _height);
//    }
}


void LauncherWindow::onEvent(XEvent *event)
{
    if ( event->xany.window == _rootWindow )
    {
        _onRootWinEvent(event);
    }
    else if ( event->xany.window == _win )
    {
        _onWinEvent(event);
    }
}

void LauncherWindow::_onRootWinEvent(XEvent *event)
{
    if ( event->type == ConfigureNotify )
    {
        if (
            event->xconfigure.window == _rootWindow &&
            (event->xconfigure.width != _width || event->xconfigure.height != _height)
           )
        {
            setNewSize(event->xconfigure.width, event->xconfigure.height);
        }
        else if ( event->xconfigure.above == _win  )
        {
            hide();
        }
    }
}

void LauncherWindow::setNewSize(int width, int height)
{
    _width = width;
    _height = height;
    XMoveResizeWindow(_dpy, _win, 0, 0, _width, _height);

    recreateBuffer();
}

void LauncherWindow::_onKeyPress(XKeyEvent *event)
{
//    // Redraw the left or right image as being pressed
//    if ( event->keycode == XKeysymToKeycode ( _dpy, XK_Left ) )
//    {
//        _prevButtonPressed = true;
//        if ( _shown )
//            _repaintOnIdle = true;
//    }
//    else if ( event->keycode == XKeysymToKeycode ( _dpy, XK_Right ) )
//    {
//        _nextButtonPressed = true;
//        if ( _shown )
//            _repaintOnIdle = true;
//    }

    bool portrait = _width < _height;

    if (event->keycode == XKeysymToKeycode(_dpy, portrait ? XK_Up : XK_Left))
        _goPrevious();
    else if (event->keycode == XKeysymToKeycode(_dpy, portrait ? XK_Down : XK_Right))
        _goNext();
    else if (_shown)
        hide();
}

void LauncherWindow::_onKeyRelease(XKeyEvent *event)
{
}

void LauncherWindow::_onMotion(XMotionEvent *event)
{
}


void LauncherWindow::onLongTap(Timeout* timeout)
{
    _longtapTimeout = 0;

    _ignoreNextButtonRelease = true;

    _categoryIconsBarVisible = true;
    _repaintOnIdle = true;
}

void LauncherWindow::_onButtonEvent(XEvent *event)
{
    if (event->type == ButtonRelease)
    {
        if (_longtapTimeout)
        {
            XEventLoop::instance()->cancelTimeout(_longtapTimeout);
            _longtapTimeout = 0;
        }
    }


    if (event->type == ButtonRelease && _ignoreNextButtonRelease)
    {
        _ignoreNextButtonRelease = false;
        return;
    }


    if (event->type == ButtonPress && _categoryIconsBarVisible)
    {
        if (
                (
                    (_width >= _height) &&
                    (event->xbutton.y > _height - _panelHeight - _categoryIconsBar->height()) &&
                    (event->xbutton.y < _height - _panelHeight)
                )
                ||
                (
                    (_width <  _height) &&
                    (event->xbutton.x > _panelHeight) &&
                    (event->xbutton.x < _panelHeight + _categoryIconsBar->height())
                )
           )
        {
            if (_categoryIcons.size() > 0)
            {
                int iconWidth = _categoryIconsBar->width() / (_categoryIcons.size() + 1);

                int iconNo;
                if (_width >= _height)
                    iconNo = event->xbutton.x / iconWidth;
                else
                    iconNo = event->xbutton.y / iconWidth;

                if (iconNo == 0)
                    MenuReader::getInstance()->sectionList()->get(_currentSection)->setIcon(0, false);
                else
                    MenuReader::getInstance()->sectionList()->get(_currentSection)->setIcon(_categoryIcons[iconNo-1], false);
            }
        }

        _ignoreNextButtonRelease = true;
        _categoryIconsBarVisible = false;
        _repaintOnIdle = true;
    }
    else if (
                ((_width >=_height) && (event->xbutton.y > _height - _panelHeight)) ||
                ((_width < _height) && (event->xbutton.x < _panelHeight))
            )
    {
        if (event->type == ButtonPress)
        {
            int sectionWidth = _panelWidth / MenuReader::getInstance()->sectionList()->getSize();
            int sectionNo = _width >= _height ?
                event->xbutton.x / sectionWidth :
                event->xbutton.y / sectionWidth;
            _currentSection = sectionNo;
            _repaintOnIdle = true;

            if (_longtapTimeout)
            {
                XEventLoop::instance()->cancelTimeout(_longtapTimeout);
                _longtapTimeout = 0;
            }
            _longtapTimeout = XEventLoop::instance()->addTimeout(1, Delegate(this, &LauncherWindow::onLongTap));
        }
    }
    else if (event->type == ButtonRelease)
    {
        // loop thru apps
        Section *currentSection = MenuReader::getInstance()->sectionList()->get ( _currentSection );
        bool aHit = FALSE;
        bool success;
        Application *app;
        for ( uint i = 0; i < currentSection->getApplicationsSize(); i++ )
        {
            app = currentSection->getApplication ( i );
            aHit = app->isAHit ( event->xbutton.x, event->xbutton.y ) ;
            if ( aHit )
            {
                success = app->execute();

                if (success)
                    showNotification( g_strconcat ("Starting ", app->getApplicationName(), NULL) );
                else
                    showNotification( "Execution failed" );
                break;
            }
        }

        if ( _shown )
            hide();
    }
}

void LauncherWindow::_onWinEvent(XEvent *event)
{
    if ( event->type == KeyPress )
    {
        _onKeyPress( & event->xkey );
    }
    else if ( event->type == KeyRelease )
    {
        _onKeyRelease( & event->xkey );
    }
    else if ( event->type == MotionNotify )
    {
        _onMotion( & event->xmotion );
    }
    else if ( event->type == ButtonPress || event->type == ButtonRelease )
    {
        _onButtonEvent(event);
    }
    else if ( event->type == Expose && event->xexpose.count < 1 )
        _repaintOnIdle = true;
}

void LauncherWindow::showNotification(const char *message)
{
    DBusMessage *call = dbus_message_new_method_call(
        "org.freedesktop.Notifications",
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications",
        "SystemNoteInfoprint"
    );

    dbus_message_append_args (call,
                              DBUS_TYPE_STRING, &message,
                              DBUS_TYPE_INVALID);

    dbus_connection_send(DBus::instance()->getConnection(), call, 0);

    dbus_message_unref(call);
}

void LauncherWindow::paint()
{
    if ( ! _shown )
        return;


    // draw background
    XCopyArea ( _dpy, Resources::instance()->wallpaper()->pixmap(), _buffer->pixmap(), _gc,
                0, 0, _width, _height, 0, 0
              );

//    Resources * resources = Resources::instance();

    // draw the current section
    Section *currentSection = MenuReader::getInstance()->sectionList()->get ( _currentSection );
    bool landscape = _width >= _height;
    currentSection->draw(
        _dpy, _buffer,
        landscape ? 0 : _panelHeight,
        0,
        landscape ? _width : _width - _panelHeight,
        landscape ? _height - _panelHeight : _height
    );



    // Drawing bottom panel
    {
        int sectionWidth = _panelWidth / MenuReader::getInstance()->sectionList()->getSize();

        XRenderComposite(_dpy, PictOpSrc,
            _panelBackground->picture(), None, _panel->picture(),
            0, 0, 0, 0,
            0, 0,
            _panelWidth, _panelHeight
        );

        XRenderComposite(_dpy, PictOpOver,
            _panelFocusLeft->picture(), None, _panel->picture(),
            0, 0, 0, 0,
            sectionWidth * _currentSection, 0,
            _panelFocusLeft->width(), _panelHeight
        );
        XRenderComposite(_dpy, PictOpOver,
            _panelFocusRight->picture(), None, _panel->picture(),
            0, 0, 0, 0,
            sectionWidth * (_currentSection+1) - _panelFocusRight->width(), 0,
            _panelFocusRight->width(), _panelHeight
        );
        XRenderComposite(_dpy, PictOpOver,
            _panelFocusMiddle->picture(), None, _panel->picture(),
            0, 0, 0, 0,
            sectionWidth * _currentSection + _panelFocusLeft->width(), 0,
            sectionWidth - _panelFocusLeft->width() - _panelFocusRight->width(), _panelHeight
        );


        XftColor fontColor = { 0, { 0xffff, 0xffff, 0xffff, 0xffff } };
        XftColor fontColorShadow = { 0, { 0x2000, 0x2000, 0x2000, 0xffff } };

        for (unsigned int i = 0; i < MenuReader::getInstance()->sectionList()->getSize(); i++)
        {
            Section* section = MenuReader::getInstance()->sectionList()->get(i);
            Image* icon = section->getIcon();

            if (icon)
            {
                int x = (sectionWidth - icon->width()) / 2;
                int y = (_panelHeight - icon->height()) / 2;

                XRenderComposite(_dpy, PictOpOver,
                    icon->picture(), None, _panel->picture(),
                    0, 0, 0, 0,
                    i * sectionWidth + x, y,
                    icon->width(), icon->height()
                );
            }
            else
            {
                const char *title = section->getNameWithPart();

                const int pad = 5;

                XRectangle rect = {
                    i * sectionWidth + pad, 0,
                    sectionWidth - 2 * pad, _panelHeight
                };
                Region clip = XCreateRegion();
                XUnionRectWithRegion(&rect, clip, clip);


                XGlyphInfo textInfo;
                XftTextExtentsUtf8(
                    _dpy, _xftFont,
                    (const FcChar8*)title, strlen(title),
                    &textInfo
                );

                XftDrawSetClip(_xftDraw, clip);

                int x = (sectionWidth - textInfo.width) / 2;
                // textInfo.y instead of textInfo.height because height includes underline elements of letters
                int y = (_panelHeight - textInfo.y) / 2 + textInfo.y;

                if (x < pad) x = pad;

                XftDrawStringUtf8(
                    _xftDraw, &fontColorShadow, _xftFont,
                    i * sectionWidth + x /*+ 1*/,
                    y + 1,
                    (const FcChar8*)title, strlen(title)
                );
                XftDrawStringUtf8(
                    _xftDraw, &fontColor, _xftFont,
                    i * sectionWidth + x,
                    y,
                    (const FcChar8*)title, strlen(title)
                );

                XDestroyRegion(clip);
            }
        }

        // Blit panel into buffer
        if (_width > _height)
        {

            // Landscape
            XCopyArea(
                _dpy, _panel->pixmap(), _buffer->pixmap(), _gc,
                0, 0, _panelWidth, _panelHeight,
                0, _height - _panelHeight
            );


            if (_categoryIconsBarVisible)
            {
                XCopyArea(
                    _dpy, _categoryIconsBar->pixmap(), _buffer->pixmap(), _gc,
                    0, 0, _categoryIconsBar->width(), _categoryIconsBar->height(),
                    0, _height - _panel->height() - _categoryIconsBar->height()
                );
            }
        }
        else
        {
            // Portrait. Rotate panel 90° and blit alongside left border

            XTransform ident = {{
                { XDoubleToFixed(1), XDoubleToFixed(0), XDoubleToFixed(0) },
                { XDoubleToFixed(0), XDoubleToFixed(1), XDoubleToFixed(0) },
                { XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(1) },
            }};

            XTransform xform = {{
                { XDoubleToFixed(0), XDoubleToFixed(1), XDoubleToFixed(0) },
                { XDoubleToFixed(-1), XDoubleToFixed(0), XDoubleToFixed(0) },
                { XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(1) },
            }};

            XRenderSetPictureTransform(_dpy, _panel->picture(), &xform);

            XRenderComposite(_dpy, PictOpSrc,
                _panel->picture(), 0, _buffer->picture(),
                -_panelHeight, 0,
                0, 0,
                0, 0,
                _panelHeight, _panelWidth
            );

            XRenderSetPictureTransform(_dpy, _panel->picture(), &ident);


            if (_categoryIconsBarVisible)
            {
                XRenderSetPictureTransform(_dpy, _categoryIconsBar->picture(), &xform);
                XRenderComposite(_dpy, PictOpSrc,
                    _categoryIconsBar->picture(), 0, _buffer->picture(),
                    -_categoryIconsBar->height(), 0,
                    0, 0,
                    _panelHeight, 0,
                    _categoryIconsBar->height(), _categoryIconsBar->width()
                );
                XRenderSetPictureTransform(_dpy, _categoryIconsBar->picture(), &ident);
            }
        }
    }

    XCopyArea(_dpy, _buffer->pixmap(), _win, _gc,
        0, 0,
        _width, _height,
        0, 0
    );
}


void LauncherWindow::onIdle()
{
    if ( _repaintOnIdle )
    {
        paint();
        _repaintOnIdle = false;
    }

    // check if the application menu file has been changed
    if ( ! _shown && MenuReader::getInstance()->hasChange())
    {
        printf("menu file has changed\n");
//        delete _sections;

        /*_sections = */MenuReader::getInstance()->processMenu();
        _currentSection = 0;

        redrawSections();
    }
}

void LauncherWindow::recreateBuffer()
{
    if (_buffer)
        delete _buffer;

    _buffer = new Image(_dpy, _width, _height, DefaultDepth(_dpy, DefaultScreen(_dpy)));
}



int checkpng(const struct dirent *ent)
{
    static const char *png = ".png";
    const char *match = strstr(ent->d_name, png);
    return match && (unsigned int)(match - ent->d_name) == strlen(ent->d_name) - strlen(png);
}

void LauncherWindow::buildCategoryIconsBar()
{
    delete _categoryIconsBar;
    for (LinkedList<Image*>::Iter i = _categoryIcons.head(); i; ++i)
        delete (*i);
    _categoryIcons.clear();

    _categoryIconsBar = new Image(_dpy, _panel->width(), _panel->height(), DefaultDepth(_dpy, DefaultScreen(_dpy)));

    XRenderComposite(_dpy, PictOpSrc,
        _panelBackground->picture(), None, _categoryIconsBar->picture(),
        0, 0, 0, 0, 0, 0,
        _categoryIconsBar->width(), _categoryIconsBar->height()
    );


    struct dirent **namelist;

    const char *dirname = Settings::instance()->categoryIconsDir();

    int count = scandir(dirname, &namelist, checkpng, alphasort);

    if (count > 0)
    {
        int iconwidth = _categoryIconsBar->width() / (count+1);

        XftDraw *xftDraw = XftDrawCreate(
            _dpy, _categoryIconsBar->pixmap(),
            DefaultVisual(_dpy, DefaultScreen(_dpy)), DefaultColormap(_dpy, DefaultScreen(_dpy))
        );

        XftColor fontColor = { 0, { 0xffff, 0xffff, 0xffff, 0xffff } };
        XftColor fontColorShadow = { 0, { 0x2000, 0x2000, 0x2000, 0xffff } };

        XGlyphInfo textInfo;
        XftTextExtentsUtf8(
            _dpy, _xftFont,
            (const FcChar8*)"no icon", 7,
            &textInfo
        );

        int x = (iconwidth - textInfo.width) / 2;
        int y = (_categoryIconsBar->height() - textInfo.y) / 2 + textInfo.y;

        XftDrawStringUtf8(
            xftDraw, &fontColorShadow, _xftFont,
            x, y + 1,
            (const FcChar8*)"no icon", 7
        );
        XftDrawStringUtf8(
            xftDraw, &fontColor, _xftFont,
            x, y,
            (const FcChar8*)"no icon", 7
        );

        XftDrawDestroy(xftDraw);


        for (int i = 0; i < count; i++)
        {
            char *filename = new char[strlen(dirname) + 1 + strlen(namelist[i]->d_name) + 1];
            strcpy(filename, dirname);
            strcat(filename, "/");
            strcat(filename, namelist[i]->d_name);
            Image *icon = new Image(_dpy, filename);
            delete[] filename;

            int x = (iconwidth - icon->width()) / 2;
            int y = (_categoryIconsBar->height() - icon->height()) / 2;

            XRenderComposite(_dpy, PictOpOver,
                icon->picture(), None, _categoryIconsBar->picture(),
                0, 0, 0, 0,
                iconwidth * (i+1) + x, y,
                icon->width(), icon->height()
            );

            free(namelist[i]);

            _categoryIcons.append(icon);
        }

        free(namelist);
    }
}
