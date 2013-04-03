//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id$

#include "Image.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Imlib2.h>



static Visual *defaultVisual = 0;
static XVisualInfo rgbaVisual;
static XRenderPictFormat *defaultFormat;
static XRenderPictFormat *rgbaFormat;
static Colormap colormap;


void requestRGBAVisual(Display *dpy)
{
    if (defaultVisual == 0)
    {
        // Initializing

        int scr = DefaultScreen(dpy);
        defaultVisual = DefaultVisual(dpy, scr);
        colormap = DefaultColormap(dpy, scr);
        defaultFormat = XRenderFindVisualFormat(dpy, defaultVisual);

        if (XMatchVisualInfo(dpy, scr, 32, TrueColor, &rgbaVisual) == 0)
        {
            fprintf(stderr, "Cannot find rgba visual\n");
            return;
        }
        rgbaFormat = XRenderFindVisualFormat(dpy, rgbaVisual.visual);
    }
}




Image::Image()
    :_dpy(0), _pixmap(0), _picture(0),
     _width(0), _height(0), _repeatType(RepeatNone),
     _filename(0)
{
}


Image::Image(Display *dpy, const char *filename)
    :_dpy(dpy), _pixmap(0), _picture(0),
     _width(0), _height(0), _repeatType(RepeatNone)
{
    _filename = strdup(filename);

    requestRGBAVisual(_dpy);

    Imlib_Image image = imlib_load_image(filename);
    if (image == 0)
    {
        fprintf(stderr, "Cannot load image: %s\n", filename);
        return;
    }

    imlib_context_set_image(image);
    _width = imlib_image_get_width();
    _height = imlib_image_get_height();


    // Making image with premultiplied alpha
    Imlib_Image premul = imlib_create_image(_width, _height);
    imlib_context_set_image(premul);
    imlib_context_set_color(0, 0, 0, 255);
    imlib_image_fill_rectangle(0, 0, _width, _height);
    imlib_context_set_blend(1);
    imlib_blend_image_onto_image(image, 0, 0, 0, _width, _height, 0, 0, _width, _height);
    imlib_image_copy_alpha_to_image(image, 0, 0);

    _pixmap = XCreatePixmap(_dpy, RootWindow(_dpy, DefaultScreen(_dpy)), _width, _height, 32);
    imlib_context_set_display(_dpy);
    imlib_context_set_colormap(colormap);
    imlib_context_set_visual(rgbaVisual.visual);
    imlib_context_set_drawable(_pixmap);
    imlib_context_set_blend(0);
    imlib_render_image_on_drawable(0, 0);

    imlib_free_image();
    imlib_context_set_image(image);
    imlib_free_image();

    _picture = XRenderCreatePicture(_dpy, _pixmap, rgbaFormat, 0, 0);
}


Image::Image(Display *dpy, int width, int height, int depth)
    :_dpy(dpy), _pixmap(0), _picture(0),
     _width(width), _height(height), _repeatType(RepeatNone),
     _filename(0)
{
    requestRGBAVisual(_dpy);

    _pixmap = XCreatePixmap(_dpy, RootWindow(_dpy, DefaultScreen(_dpy)), _width, _height, depth);

    _picture = XRenderCreatePicture(_dpy, _pixmap,
        depth == 32 ? rgbaFormat : defaultFormat,
        0, 0
    );
}


Image::~Image()
{
    if (_picture) XRenderFreePicture(_dpy, _picture);
    if (_pixmap) XFreePixmap(_dpy, _pixmap);

    free(_filename);
}



void Image::setRepeatType(int repeatType)
{
    if (! _picture) return;

    XRenderPictureAttributes pictureAttrs;
    pictureAttrs.repeat = repeatType;
    XRenderChangePicture(_dpy, _picture, CPRepeat, &pictureAttrs);
}


void Image::clear()
{
    GC gc = XCreateGC(_dpy, _pixmap, 0, 0);
    XSetGraphicsExposures(_dpy, gc, false);
    XFillRectangle(_dpy, _pixmap, gc, 0, 0, _width, _height);
    XFreeGC(_dpy, gc);
}
