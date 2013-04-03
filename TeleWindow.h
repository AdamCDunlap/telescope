//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: TeleWindow.h 163 2011-02-25 08:18:37Z mitrandir $

// TeleWindow - main window of Telescope

#ifndef __TELESCOPE__TELEWINDOW_H
#define __TELESCOPE__TELEWINDOW_H

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>

#include <Imlib2.h>

#include "LinkedList.h"
#include "Mappings.h"

#include "XEventHandler.h"
#include "XIdleTask.h"


class Image;
class Thumbnail;

class TeleWindow: public XEventHandler, public XIdleTask
{
    private:
        Display *_dpy;
        Window _rootWindow;
        Window _win;

        LinkedList<Thumbnail*> _thumbnails;
        bool _thumbnailsListDirty;

        int _width;
        int _height;
        bool _shown;

        KeyCode _hotKeyCode;
        bool _hotKeyPressed;
        KeyCode _selectKeyCode;

        GC _gc;

        XftFont *_xftFont;
        Image* _buffer;


        XRenderColor _borderColor;
        XRenderColor _borderActiveColor;



        int _scrollBaseX, _scrollBaseY;
        int _scrollX, _scrollY;
        bool _buttonPressed;
        bool _wasScrolling;
        int _buttonPressX, _buttonPressY;
        bool _repaintOnIdle;


        Thumbnail *_activeThumbnail;


        Mappings _mappings;


        void onHotKeyPress();
        void onHotKeyRelease();


        void onButtonPress(XButtonEvent *event);
        void onButtonRelease(XButtonEvent *event);
        void onButtonMotion(XMotionEvent *event);

        void setNewSize(int width, int height);
        void recreateBuffer();

        void animate(Thumbnail *thumb, bool toSmall);

        void blitThumb(Thumbnail *thumbnail);
        void blitBuffer();

        Thumbnail* findThumbnailByCoords(
            Thumbnail *orig,
            int direction
        );

    public:
        TeleWindow(Display *dpy);
        virtual ~TeleWindow();

        Display* display();
        Window window();

        bool show();
        void hide();
        bool shown();

        int scrollX()
        { return _scrollX; }
        int scrollY()
        { return _scrollY; }


        void updateThumbnailsList();
        void layoutThumbnails();

        Thumbnail* activeThumbnail() { return _activeThumbnail; }

        void removeThumbnail(Thumbnail *thumb);

        void onRootEvent(XEvent *event);
        void onTeleWindowEvent(XEvent *event);

        void paint();

        void onThumbRedrawed(Thumbnail *thumb);

        void internalCommand(const char *action);

        void markThumbnailsListDirty();

        XftFont* xftFont() { return _xftFont; }

        void showDesktop();


        virtual void onEvent(XEvent *event);
        virtual void onIdle();
};


#endif
