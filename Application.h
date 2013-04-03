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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <X11/Xlib.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>

#include <glib.h>

class Image;


class Application
{
public:
    Application(Display *dpy, const gchar *filename);
    ~Application();
    const gchar* getIcon();
    const gchar* getApplicationName();
    bool execute();
    bool isValid();
    void setPosition(int x, int y);
//    Picture draw(Display *dpy);
    bool isAHit(int x, int y);
    int x() { return _x; }
    int y() { return _y; }

    static int width() { return _pixmapWidth; }
    static int height() { return _pixmapHeight; }

    Image* image() { return _image; }

private:
    gchar * _filename;
    gchar *_icon;
    gchar *_iconPath;
    gchar *_appName;
    gchar *_executable;
    gchar *_service;

    bool _runInTerminal;
    bool _isValid;
    int _x, _y;

    static int _pixmapWidth;
    static int _pixmapHeight;

    bool _executeService();
    bool _executeNormally();
    bool _executeInTerminal();

    Display * _dpy;
    static XftFont *_xftFont;

    Image* _image;
    void createPicture();
};

#endif // APPLICATION_H

