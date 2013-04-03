//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: Thumbnail.h 163 2011-02-25 08:18:37Z mitrandir $

// Thumbnail - represents single window preview

#ifndef __TELESCOPE__THUMBNAIL_H
#define __TELESCOPE__THUMBNAIL_H

#include <X11/Xlib.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>


class TeleWindow;
class Image;

class Thumbnail
{
    private:
        Display *_dpy;

        TeleWindow *_teleWindow;
        Window _clientWindow;
        char *_title;
        char *_clientClass;

        Image* _image;
        GC _gc;
        XftDraw *_xftDraw;

        Damage _damage;

        Picture _clientPict;

        int _depth;
        bool _previewValid;
        bool _previewOnceDrawn;

        int _x, _y;
        int _width, _height;
        int _fitX, _fitY;
        int _fitWidth, _fitHeight;

        int _clientWidth, _clientHeight;
        int _clientScaledWidth, _clientScaledHeight;

        int _clientDecoX, _clientDecoY;
        int _clientDecoXScaled, _clientDecoYScaled;

        int _clientOffsetX, _clientOffsetY;

        bool _clientDestroyed;

        bool _minimized;

#ifdef MAEMO4
        bool _isOssoMediaPlayer;
        bool _isLiqBase;
#endif


        void onResize();
        void onClientResize(XEvent *event);

    public:
        Thumbnail(TeleWindow *teleWindow, Window clientWindow);
        ~Thumbnail();

//        Window window();
        Window clientWindow();

        Image* image() { return _image; }

        void setClientDestroyed(bool clientDestroyed) { _clientDestroyed = clientDestroyed; }

        const char* title() { return _title; }
        const char* clientClass() { return _clientClass; }

        void setGeometry(int x, int y, int w, int h);
        void fitIn(int rx, int ry, int rwidth, int rheight);
        void tryFitIn(int rx, int ry, int rwidth, int rheight,
            int *x, int *y, int *width, int *height);

        int x() { return _x; }
        int y() { return _y; }
        int width() { return _width; }
        int height() { return _height; }

        int realWidth() { return _clientScaledWidth; }
        int realHeight() { return _clientScaledHeight; }

        Picture clientPicture() { return _clientPict; }

        int clientDecoX() { return _clientDecoX; }
        int clientDecoY() { return _clientDecoY; }

        bool inside(int x, int y);

#ifdef MAEMO4
        bool isOssoMediaPlayer() { return _isOssoMediaPlayer; }
        bool isLiqBase() { return _isLiqBase; }
#endif

        bool mustBeIconifiedBeforeTelescope();

        void onClientEvent(XEvent *event);


        void drawPreview();
        void redraw();

        bool handleMousePress(int x, int y);

        void switchToClient();
        void closeClient();
        void minimize();
};


#endif
