//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: Resources.h 163 2011-02-25 08:18:37Z mitrandir $

// Resources - loads and ownes all common resources

// Singleton

#ifndef __TELESCOPE__RESOURCES_H
#define __TELESCOPE__RESOURCES_H

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

class Image;


class Resources
{
    private:
        static Resources *_instance;

        Display *_dpy;

        Image* _headerLeft;
        Image* _headerRight;
        Image* _headerMiddle;
        Image* _headerLeftSelected;
        Image* _headerRightSelected;
        Image* _headerMiddleSelected;
        Image* _brokenPattern;

        XRenderColor _borderColor;
        XRenderColor _borderActiveColor;

        Image* _textBackground;


        XRenderColor _backgroundColor;

        Image* _wallpaper;

    public:
        Resources(Display *dpy);
        ~Resources();

        static Resources* instance() { return _instance; }


        Image* headerLeft()     { return _headerLeft; }
        Image* headerRight()    { return _headerRight; }
        Image* headerMiddle()   { return _headerMiddle; }
        Image* headerLeftSelected()     { return _headerLeftSelected; }
        Image* headerRightSelected()    { return _headerRightSelected; }
        Image* headerMiddleSelected()   { return _headerMiddleSelected; }

        Image* brokenPattern()   { return _brokenPattern; }

        Image* getTextBackground() { return _textBackground; }


        const XRenderColor* borderColor() { return &_borderColor; }
        const XRenderColor* borderActiveColor() { return &_borderActiveColor; }


        Image* wallpaper() { return _wallpaper; }
        void reloadWallpaper();
};


#endif
