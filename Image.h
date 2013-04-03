//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id$

// Image - class representing pixmap & xrender picture pair

#ifndef __TELESCOPE__IMAGE_H
#define __TELESCOPE__IMAGE_H

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>


class Image
{
    private:
        Display *_dpy;
        Pixmap _pixmap;
        Picture _picture;

        int _width;
        int _height;
        int _repeatType;

        char *_filename;


    public:
        Image();
        Image(Display *dpy, const char *filename);
        Image(Display *dpy, int width, int height, int depth = 32);
        ~Image();

        bool valid() const { return _pixmap != 0; }

        int width() const { return _width; }
        int height() const { return _height; }
        int repeatType() const { return _repeatType; }
        void setRepeatType(int repeatType);

        Display* display() const { return _dpy; }
        Pixmap pixmap() const { return _pixmap; }
        Picture picture() const { return _picture; }

        void clear();

        const char* filename() const { return _filename; }
};


#endif
