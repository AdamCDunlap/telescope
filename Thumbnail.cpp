//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: Thumbnail.cpp 181 2011-04-29 04:25:02Z mitrandir $

#include "Thumbnail.h"

#include <math.h>
#include <stdlib.h>

#include "TeleWindow.h"
#include "XTools.h"
#include "Settings.h"
#include "Resources.h"
#include "Image.h"


Thumbnail::Thumbnail(TeleWindow *teleWindow, Window clientWindow)
{
    _teleWindow = teleWindow;
    _dpy = teleWindow->display();
    _clientWindow = clientWindow;

    _depth = DefaultDepth(_dpy, DefaultScreen(_dpy));

    _title = XTools::windowTitle_alloc(_clientWindow);
    _clientClass = XTools::windowClass_alloc(_clientWindow);

#ifdef MAEMO4
    _isOssoMediaPlayer = strcmp(_clientClass, "mediaplayer-ui") == 0;
    _isLiqBase = strncmp(_clientClass, "liq", 3) == 0;
#endif

    _image = 0;
    _gc = 0;
    _xftDraw = 0;


    _clientDestroyed = false;

    XSelectInput(_dpy, _clientWindow, StructureNotifyMask | PropertyChangeMask);


    _damage = XDamageCreate(_dpy, _clientWindow, XDamageReportNonEmpty);


    XWindowAttributes attrs;
    XGetWindowAttributes(_dpy, _clientWindow, &attrs);

#ifdef DESKTOP
    Window root;
    Window parent;
    Window *children;
    unsigned int nchildren;
    XQueryTree(_dpy, _clientWindow, &root, &parent, &children, &nchildren);
    XFree(children);

    XWindowAttributes decoAttrs;
    XGetWindowAttributes(_dpy, parent, &decoAttrs);

    _clientDecoX = decoAttrs.x;
    _clientDecoY = decoAttrs.y;
#else
    #ifdef MAEMO4
        _clientDecoX = attrs.x;
        _clientDecoY = attrs.y;
    #else
        #error Unknown window manager
    #endif
#endif


    _previewValid = false;
    _previewOnceDrawn = false;


    // First setGeometry call will compare this with new dimensions
    _width = -1;
    _height = -1;


    _minimized = XTools::checkIfWindowMinimized(_clientWindow);


    XRenderPictureAttributes pa;
    pa.subwindow_mode = IncludeInferiors;

    _clientPict = XRenderCreatePicture(_dpy, _clientWindow, XTools::xrenderFormat(), CPSubwindowMode, &pa);
}

Thumbnail::~Thumbnail()
{
    if (_xftDraw)
        XftDrawDestroy(_xftDraw);

    if (_gc)
        XFreeGC(_dpy, _gc);

    delete _image;

    if (! _clientDestroyed)
    {
        XSelectInput(_dpy, _clientWindow, 0);
        XDamageDestroy(_dpy, _damage);
        XRenderFreePicture(_dpy, _clientPict);
    }

    free(_title);
    free(_clientClass);
}


Window Thumbnail::clientWindow()
{
    return _clientWindow;
}


void Thumbnail::setGeometry(int x, int y, int w, int h)
{
    _x = x;
    _y = y;

    if (_width != w || _height != h)
    {
        _width = w;
        _height = h;
        onResize();
    }
    else
    {
        _width = w;
        _height = h;
    }
}

bool Thumbnail::inside(int x, int y)
{
    return x >= _x && y >= _y && x < _x + _width && y < _y + _height;
}

void Thumbnail::onClientEvent(XEvent *event)
{
    if (event->type == XTools::damageEventBase() + XDamageNotify)
    {
        _previewValid = false;
        if (_teleWindow->shown())
        {
            drawPreview();
            _teleWindow->onThumbRedrawed(this);
        }

        XDamageSubtract(_dpy, ((XDamageNotifyEvent*)event)->damage, None, None);
    }
    else if (event->type == ConfigureNotify)
    {
        onClientResize(event);
    }
//    else if (event->type == UnmapNotify)
//    {
//        _teleWindow->updateThumbnailsList();
//    }
    else if (event->type == PropertyNotify)
    {
        if (event->xproperty.atom == XTools::_NET_WM_NAME ||
            event->xproperty.atom == XTools::WM_NAME)
        {
            free(_title);
            _title = XTools::windowTitle_alloc(_clientWindow);

            redraw();
            if (_teleWindow->shown())
                _teleWindow->onThumbRedrawed(this);
        }
        else if (event->xproperty.atom == XTools::WM_STATE);
        {
            _minimized = XTools::checkIfWindowMinimized(_clientWindow);
        }
    }
//    else
//        printf("Thumbnail: unknown client event (%d)\n", event->type);
}



void Thumbnail::tryFitIn(int rx, int ry, int rwidth, int rheight,
    int *x, int *y, int *width, int *height)
{
    int borderWidth = Settings::instance()->borderWidth();
    int headerHeight = Resources::instance()->headerMiddle()->height();

    XWindowAttributes attrs;
    XGetWindowAttributes(_dpy, _clientWindow, &attrs);

    _clientWidth = attrs.width;
    _clientHeight= attrs.height;

    // Thumb width if limited in horizontal direction
    int horWidth = rwidth;

    // Thumb width if limited in vertical direction
    int verWidth = (rheight - headerHeight - borderWidth);
    verWidth = (int)round((float)verWidth * _clientWidth / _clientHeight);
    verWidth += 2 * borderWidth;

    int newWidth;
    int newHeight;

    if (horWidth < verWidth)
    {
        newWidth = horWidth;
        newHeight = (rwidth - 2 * borderWidth);
        newHeight = (int)round((float)newHeight * _clientHeight / _clientWidth);
        newHeight += headerHeight + borderWidth;
    }
    else
    {
        newWidth = verWidth;
        newHeight = rheight;
    }

    *x = rx + (rwidth - newWidth) / 2;
    *y = ry + (rheight-newHeight) / 2;
    *width = newWidth;
    *height = newHeight;
}


void Thumbnail::fitIn(int rx, int ry, int rwidth, int rheight)
{
    _fitX = rx; _fitY = ry;
    _fitWidth = rwidth; _fitHeight = rheight;

    int x, y, width, height;

    tryFitIn(rx, ry, rwidth, rheight,
             &x, &y, &width, &height);

    setGeometry(x, y, width, height);
}


void Thumbnail::onResize()
{
    Image* oldImage = _image;

    if (_image)
    {
        XftDrawDestroy(_xftDraw);
        XFreeGC(_dpy, _gc);
    }

    _image = new Image(_dpy, _width, _height);
    _gc = XCreateGC(_dpy, _image->pixmap(), 0, 0);
    XSetGraphicsExposures(_dpy, _gc, false);
    _xftDraw = XftDrawCreate(_dpy, _image->pixmap(), XTools::rgbaVisual()->visual,
        DefaultColormap(_dpy, DefaultScreen(_dpy)));


    XWindowAttributes attrs;
    XGetWindowAttributes(_dpy, _clientWindow, &attrs);

    _clientWidth = attrs.width;
    _clientHeight= attrs.height;

#ifdef MAEMO4
    _clientDecoX = attrs.x;
    _clientDecoY = attrs.y;
#endif

    int borderWidth = Settings::instance()->borderWidth();
    int headerHeight = Resources::instance()->headerMiddle()->height();


    int oldClientScaledWidth = _clientScaledWidth;
    int oldClientScaledHeight= _clientScaledHeight;

    _clientScaledWidth = _width - 2 * borderWidth;
    _clientScaledHeight = _height - headerHeight - borderWidth;

    double scale = _clientScaledWidth;
    scale /= _clientWidth;

    _clientDecoXScaled = (int)round(scale * _clientDecoX);
    _clientDecoYScaled = (int)round(scale * _clientDecoY);

    _clientOffsetX = borderWidth;
    _clientOffsetY = headerHeight;

    XTransform xform = {{
        { XDoubleToFixed(1.0/scale), XDoubleToFixed(0), XDoubleToFixed(0) },
        { XDoubleToFixed(0), XDoubleToFixed(1.0/scale), XDoubleToFixed(0) },
        { XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(1) },
    }};


    XRenderSetPictureTransform(_dpy, _clientPict, &xform);


    if (_minimized && oldImage != 0)
    {
        // Workaround for corner case: if this function is called when
        // client is minimized, we need to rescale cached pixmap, because
        // can't grab new picture from window
        // Yes, this function MAY be called when client is minimised - for
        // example when thumbnail needs to be resized because number of
        // windows changed


        Image* temp = new Image(_dpy, _clientScaledWidth, _clientScaledHeight, 32);


        XTransform xform = {{
            { XDoubleToFixed(((double)oldClientScaledWidth)/_clientScaledWidth), XDoubleToFixed(0), XDoubleToFixed(0) },
            { XDoubleToFixed(0), XDoubleToFixed(((double)oldClientScaledHeight)/_clientScaledHeight), XDoubleToFixed(0) },
            { XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(1) }
        }};

        XRenderSetPictureTransform(_dpy, oldImage->picture(), &xform);
        XRenderComposite(_dpy, PictOpSrc,
            oldImage->picture(), 0, temp->picture(),
            _clientOffsetX*_clientScaledWidth/oldClientScaledWidth,
            _clientOffsetY*_clientScaledHeight/oldClientScaledHeight,
            0, 0,
            0, 0,
            _clientScaledWidth, _clientScaledHeight
        );


        XCopyArea(_dpy, temp->pixmap(), _image->pixmap(), _gc,
            0, 0,
            _clientScaledWidth, _clientScaledHeight,
            _clientOffsetX, _clientOffsetY
        );

        delete temp;
    }

    _previewValid = false;


    delete oldImage;


    redraw();
}


void Thumbnail::onClientResize(XEvent *event)
{
    fitIn(_fitX, _fitY, _fitWidth, _fitHeight);
}


void Thumbnail::drawPreview()
{
    if (! _previewValid)
    {
        if (! _minimized)
        {
            XRenderComposite(_dpy, PictOpSrc,
                    _clientPict, None, _image->picture(),
                    _clientDecoXScaled-_clientDecoX, _clientDecoYScaled - _clientDecoY,
                    0, 0,
                    _clientOffsetX, _clientOffsetY,
                    _clientScaledWidth, _clientScaledHeight);

            _previewOnceDrawn = true;
        }
        else
        {
            if (! _previewOnceDrawn)
            {
                XRenderComposite(_dpy, PictOpSrc,
                    Resources::instance()->brokenPattern()->picture(), None, _image->picture(),
                    0, 0,
                    0, 0,
                    _clientOffsetX, _clientOffsetY,
                    _clientScaledWidth, _clientScaledHeight
                );
            }
        }

        _previewValid = true;
    }
}


void Thumbnail::redraw()
{
    int borderWidth = Settings::instance()->borderWidth();
    int headerHeight = Resources::instance()->headerMiddle()->height();
    int headerLeftWidth = Resources::instance()->headerLeft()->width();
    int headerRightWidth = Resources::instance()->headerRight()->width();

    int w = _clientScaledWidth;
    int h = _clientScaledHeight;

    bool selected = (!Settings::instance()->disableSelection())
        && this == _teleWindow->activeThumbnail();
    const XRenderColor *borderColor = selected ?
        Resources::instance()->borderActiveColor() :
        Resources::instance()->borderColor();

    Picture left = selected ?
        Resources::instance()->headerLeftSelected()->picture() :
        Resources::instance()->headerLeft()->picture();
    Picture right = selected ?
        Resources::instance()->headerRightSelected()->picture() :
        Resources::instance()->headerRight()->picture();
    Picture middle = selected ?
        Resources::instance()->headerMiddleSelected()->picture() :
        Resources::instance()->headerMiddle()->picture();

    XRenderComposite(_dpy, PictOpSrc,
        left, None, _image->picture(),
        0, 0,
        0, 0,
        _clientOffsetX - borderWidth,
        _clientOffsetY - headerHeight,
        headerLeftWidth, headerHeight
    );

    XRenderComposite(_dpy, PictOpSrc,
        right, None, _image->picture(),
        0, 0,
        0, 0,
        _clientOffsetX + w + borderWidth - headerRightWidth,
        _clientOffsetY - headerHeight,
        headerRightWidth, headerHeight
    );

    XRenderComposite(_dpy, PictOpSrc,
        middle, None, _image->picture(),
        0, 0,
        0, 0,
        _clientOffsetX - borderWidth + headerLeftWidth,
        _clientOffsetY - headerHeight,
        w + 2 * borderWidth - headerLeftWidth - headerRightWidth,
        headerHeight
    );

    // Left border
    XRenderFillRectangle(_dpy, PictOpSrc, _image->picture(), borderColor,
        _clientOffsetX - borderWidth,
        _clientOffsetY,
        borderWidth, h
    );
    // Right border
    XRenderFillRectangle(_dpy, PictOpSrc, _image->picture(), borderColor,
        _clientOffsetX + w,
        _clientOffsetY,
        borderWidth, h
    );
    // Bottom border
    XRenderFillRectangle(_dpy, PictOpSrc, _image->picture(), borderColor,
        _clientOffsetX - borderWidth,
        _clientOffsetY + h,
        w + 2*borderWidth, borderWidth
    );


    // Drawing title text

    XftColor fontColor;
    fontColor.pixel = 0;
    fontColor.color.red     = 0xffff;
    fontColor.color.green   = 0xffff;
    fontColor.color.blue    = 0xffff;
    fontColor.color.alpha   = 0xffff;

    XRectangle rect = {
        _clientOffsetX - borderWidth + Settings::instance()->textLeftMargin(),
        _clientOffsetY - headerHeight,
        _clientScaledWidth + 2*borderWidth - Settings::instance()->textLeftMargin() - Settings::instance()->textRightMargin(),
        headerHeight
    };
    Region clip = XCreateRegion();
    XUnionRectWithRegion(&rect, clip, clip);

    XftDrawSetClip(_xftDraw, clip);

    XftDrawStringUtf8(_xftDraw, &fontColor, _teleWindow->xftFont(), 
        _clientOffsetX - borderWidth + Settings::instance()->textLeftMargin(),
        _clientOffsetY + Settings::instance()->textYOffset(),
        (const FcChar8*)_title,
        strlen(_title)
    );

    XDestroyRegion(clip);


    drawPreview();
}


void Thumbnail::switchToClient()
{
    XTools::switchToWindow(_clientWindow);
}


void Thumbnail::closeClient()
{
    XTools::closeWindow(_clientWindow);
}


void Thumbnail::minimize()
{
    XTools::minimize(_clientWindow);
}



bool Thumbnail::handleMousePress(int x, int y)
{
    int closeButtonXSpan = Settings::instance()->closeButtonXSpan();
    int closeButtonYSpan = Settings::instance()->closeButtonYSpan();

    int xmin = _x + _clientOffsetX + _clientScaledWidth - closeButtonXSpan;
    int xmax = xmin + closeButtonXSpan;
    int ymin = _y + _clientOffsetY - Resources::instance()->headerMiddle()->height();
    int ymax = ymin + closeButtonYSpan;

    if (xmin < x && x < xmax && ymin < y && y < ymax)
    {
        closeClient();
        return true;
    }
    else
        if (_x < x && x < _x + _width &&
            _y < y && y < _y + _height)
        {
            switchToClient();
            _teleWindow->hide();
            return true;
        }

    return false;
}


bool Thumbnail::mustBeIconifiedBeforeTelescope()
{
#ifdef MAEMO4
    return isOssoMediaPlayer() || isLiqBase();
#else
    return false;
#endif
}
