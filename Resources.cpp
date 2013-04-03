//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: Resources.cpp 163 2011-02-25 08:18:37Z mitrandir $

#include "Resources.h"

#include <stdio.h>
#include <string.h>

#include <Imlib2.h>

#include <sys/stat.h>

#include "XTools.h"
#include "constant.h"
#include "Settings.h"

#include "Image.h"

Resources* Resources::_instance = 0;


Resources::Resources(Display *dpy): _dpy(dpy)
{
    if (_instance != 0)
        printf("Resources singleton created twice!\n");

    _instance = this;

    _wallpaper = 0;


// Loading header images


    _headerLeft = new Image(_dpy, Settings::instance()->headerLeftFilename());
    _headerRight = new Image(_dpy, Settings::instance()->headerRightFilename());
    _headerMiddle = new Image(_dpy, Settings::instance()->headerMiddleFilename());
    _headerMiddle->setRepeatType(RepeatNormal);

    _headerLeftSelected = new Image(_dpy, Settings::instance()->headerLeftSelectedFilename());
    _headerRightSelected = new Image(_dpy, Settings::instance()->headerRightSelectedFilename());
    _headerMiddleSelected = new Image(_dpy, Settings::instance()->headerMiddleSelectedFilename());
    _headerMiddleSelected->setRepeatType(RepeatNormal);

    _brokenPattern = new Image(_dpy, Settings::instance()->brokenPatternFilename());
    _brokenPattern->setRepeatType(RepeatNormal);

    _textBackground = new Image(_dpy, Settings::instance()->textBackgroundFilename());


    XRenderParseColor(_dpy, Settings::instance()->borderColor(), &_borderColor);
    XRenderParseColor(_dpy, Settings::instance()->borderActiveColor(), &_borderActiveColor);

    XRenderParseColor(_dpy, Settings::instance()->backgroundColor(), &_backgroundColor);

    reloadWallpaper();
}


Resources::~Resources()
{
    delete _headerLeft;
    delete _headerRight;
    delete _headerMiddle;
    delete _headerLeftSelected;
    delete _headerRightSelected;
    delete _headerMiddleSelected;

    delete _textBackground;

    delete _wallpaper;
}




void Resources::reloadWallpaper()
{
    if (!_wallpaper)
        delete _wallpaper;

    Window rootWindow = XTools::rootWindow();
    XWindowAttributes attrs;
    XGetWindowAttributes(_dpy, rootWindow, &attrs);
    int width = attrs.width;
    int height = attrs.height;

    _wallpaper = new Image(_dpy, width, height, attrs.depth);

    printf("Loading background from '%s'\n", Settings::instance()->backgroundFilename());
    printf("Background mode: %d\n", Settings::instance()->backgroundMode());

    imlib_context_set_display(_dpy);

    int bgXpos = 0;
    int bgYpos = 0;
    Imlib_Image background = imlib_load_image(Settings::instance()->backgroundFilename());
    if (background == 0)
    {
        printf("Cannot load background\n");
        printf("rgb = (%d, %d, %d)\n",
            _backgroundColor.red   / 256,
            _backgroundColor.green / 256,
            _backgroundColor.blue  / 256
        );
        background = imlib_create_image(width, height);
        imlib_context_set_image(background);
        imlib_context_set_color(
            _backgroundColor.red   / 256,
            _backgroundColor.green / 256,
            _backgroundColor.blue  / 256,
            255
        );
        imlib_image_fill_rectangle(0, 0, width, height);
    }
    else
    {
        Imlib_Image bgToDraw = 0;

        imlib_context_set_image(background);
        switch (Settings::instance()->backgroundMode())
        {
            case Settings::Stretched:
            {
                bgToDraw = imlib_create_cropped_scaled_image(
                    0, 0,
                    imlib_image_get_width(), imlib_image_get_height(),
                    width, height
                );
                break;
            }

            case Settings::Centered:
            {
                // Hildon's logic is not so easy in this case

//                bgToDraw = imlib_create_cropped_image(
//                    (imlib_image_get_width() - width) / 2,
//                    (imlib_image_get_height()- height) / 2,
//                    width, height
//                );

                double scalex = 1.0;
                double scaley = 1.0;

                if (imlib_image_get_width() > width * 2)
                    scalex = (double)imlib_image_get_width() / 2.0 / width;
                if (imlib_image_get_height() > height * 2)
                    scaley = (double)imlib_image_get_height() / 2.0 / height;

                double scale = scalex;
                if (scaley > scale)
                    scale = scaley;

                int newwidth = (int)(scale * width);
                int newheight= (int)(scale * height);

                bgToDraw = imlib_create_cropped_scaled_image(
                    (imlib_image_get_width() - newwidth) / 2,
                    (imlib_image_get_height()- newheight)/ 2,
                    newwidth, newheight,
                    width, height
                );

                break;
            }

            case Settings::Scaled: case Settings::Cropped:
            {
                double scalex = (double)width / imlib_image_get_width();
                double scaley = (double)height / imlib_image_get_height();
                double scale = scalex;

                if (Settings::instance()->backgroundMode() == Settings::Scaled)
                {
                    if (scaley < scale) scale = scaley;
                }
                else
                {
                    if (scaley > scale) scale = scaley;
                }

                int newwidth = (int)(imlib_image_get_width() * scale);
                int newheight= (int)(imlib_image_get_height() * scale);

                bgToDraw = imlib_create_cropped_scaled_image(
                    0, 0,
                    imlib_image_get_width(), imlib_image_get_height(),
                    newwidth, newheight
                );

                bgXpos = (width - newwidth) / 2;
                bgYpos = (height - newheight) / 2;
                break;
            }

            default:
                printf("Unknown background mode: %d\n", Settings::instance()->backgroundMode());
        }

        imlib_free_image();

        imlib_context_set_image(bgToDraw);
    }

    imlib_context_set_drawable(_wallpaper->pixmap());
    imlib_context_set_visual(DefaultVisual(_dpy, DefaultScreen(_dpy)));

    imlib_render_image_on_drawable(bgXpos, bgYpos);

    imlib_free_image();
}
