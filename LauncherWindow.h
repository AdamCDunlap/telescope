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

#ifndef __TELESCOPE__LAUNCHERWINDOW_H
#define __TELESCOPE__LAUNCHERWINDOW_H

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>

#include <Imlib2.h>
#include "SectionList.h"

#include "LinkedList.h"

#include "XEventHandler.h"
#include "XIdleTask.h"


class Image;
class Timeout;

class LauncherWindow: public XEventHandler, public XIdleTask
{
private:
    static LauncherWindow *_instance;

    Display *_dpy;
    Window _rootWindow;
    Window _win;

    bool _breakEventLoop;

    int _width;
    int _height;
    bool _shown;

    GC _gc;

    Image *_buffer;

//    SectionList *_sections;
    uint _currentSection;


    Image* _panel;
    int _panelWidth, _panelHeight;
    Image* _panelBackground;
    Image* _panelFocusLeft;
    Image* _panelFocusRight;
    Image* _panelFocusMiddle;

    bool _categoryIconsBarVisible;
    Image* _categoryIconsBar;
    LinkedList<Image*> _categoryIcons;

    Timeout *_longtapTimeout;
    void onLongTap(Timeout* timeout);

    XftDraw *_xftDraw;
    XftFont *_xftFont;


    bool _ignoreNextButtonRelease;
    bool _repaintOnIdle;

    void reloadBackground();
    void recreateBuffer();
    void redrawSections();

    void blitBuffer();

    void showNotification(const char *message);

    void _goPrevious();
    void _goNext();

    void _onRootWinEvent(XEvent *event);
    void _onWinEvent(XEvent *event);
    void _onKeyPress(XKeyEvent *event);
    void _onKeyRelease(XKeyEvent *event);
    void _onMotion(XMotionEvent *event);
    void _onButtonEvent(XEvent *event);

    void setNewSize(int width, int height);

public:
    static LauncherWindow* instance() { return _instance; }

    LauncherWindow(Display *dpy/*, SectionList *list*/);
    virtual ~LauncherWindow();

    Display* display();
    Window window();
    Picture picture();

    void show();
    void hide();
    bool shown();
    void quit();

    void paint();

    virtual void onIdle();
    virtual void onEvent(XEvent *event);


    void buildCategoryIconsBar();
};


#endif
