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

#include <libintl.h>
#include <locale.h>

#include "Section.h"
#include "constant.h"
#include "XTools.h"

#include "Image.h"

XftFont *Section::_xftFont = 0;

Section::Section(Display *dpy, const gchar *name)
    :_dpy(dpy), _icon(0), _iconOwned(false), _iconChangedCallback(0)
{
    setName(name);
    _partNo = 1;
    _applications = g_ptr_array_new();
}

Section::~Section()
{
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Section dtor");

    for (guint i = 0; i < _applications->len; i++)
    {
        delete getApplication(i);
    }
    g_ptr_array_free(_applications, TRUE);
    g_free(_name);

    if ( _xftFont != 0 )
    {
        XftFontClose( _dpy, _xftFont );
        _xftFont = 0;
    }


    if (_iconOwned)
        delete _icon;
}

void Section::setPart(guint partNo)
{
    _partNo = partNo;
}

guint Section::getPart()
{
    return _partNo;
}

void Section::setName(const gchar *name)
{
    _name = g_strdup(name);

//    char* upcase = g_utf8_strup(name, -1);
//
//    if (upcase && strstr(upcase, "UTIL")) _icon = new Image(_dpy, "/home/user/telescope/panel-icons/games.png");
//    if (upcase && strstr(upcase, "COMMU")) _icon = new Image(_dpy, "/home/user/telescope/panel-icons/comm.png");
//    if (upcase && strstr(upcase, "EXTRA")) _icon = new Image(_dpy, "/home/user/telescope/panel-icons/extras.png");
//    if (upcase && strstr(upcase, "INTERN")) _icon = new Image(_dpy, "/home/user/telescope/panel-icons/web.png");
//    if (upcase && strstr(upcase, "SETTI")) _icon = new Image(_dpy, "/home/user/telescope/panel-icons/settings.png");
//
//    g_free(upcase);
}

const gchar * Section::getName()
{
    return _name;
}


void Section::setIcon(Image *icon, bool own)
{
    printf("setIcon(%p, %d)\n", icon, own);
    if (_iconOwned)
        delete _icon;

    _icon = icon;
    _iconOwned = own;

    if (_iconChangedCallback)
        _iconChangedCallback(this, _iconChangedCallbackData);
}


gchar * Section::getNameWithPart()
{
    if (_partNo != 1)
        return g_strdup_printf("%s — part %u", gettext( _name ), _partNo);
    else
        return g_strdup( gettext( _name ) );
}

Application * Section::getApplication(guint index)
{
    return (Application *) g_ptr_array_index(_applications, index);
}

void Section::addApplication(Application *app)
{
    g_ptr_array_add(_applications, (gpointer) app);
}

guint Section::getApplicationsSize()
{
    return _applications->len;
}

void Section::draw(Display *dpy, Image* image, int x, int y, int width, int height )
{
    // position the applications and draw them
    uint col, row;
    col = row = 0;
    Application * app;


    bool landscape = width >= height;

    uint cols = landscape ? NUM_COLS : NUM_ROWS; // (width - 2 * xborder + xmingap) / (Application::width() + xmingap);
    uint rows = landscape ? NUM_ROWS : NUM_COLS;
    //int xgap = (width - 2 * xborder - Application::width() * cols) / (cols - 1);
    int xgap = (width / cols - Application::width()) / 2;

    //int ygap = 36;
    int ygap = (height / rows - Application::height()) / 2;

    for ( uint i = 0; i < _applications->len; i++ )
    {
        if ( col >= cols )
        {
            col = 0;
            row++;
        }

//        if ( row >= NUM_ROWS )
//            break;

        // draw the app icon
        app = getApplication ( i );
        app->setPosition(
            x + col * width / cols + xgap,
            y + row * height / rows + ygap
        );

        // blit the icon into the section pixmap
        XRenderComposite ( _dpy, PictOpOver,
                           app->image()->picture(), None, image->picture(),
                           0, 0,
                           0, 0,
                           app->x(), app->y(),
                           app->width(), app->height());

        col++;
    }
}




void Section::setIconChangedCallback(IconChangedCallback callback, void *data)
{
    _iconChangedCallback = callback;
    _iconChangedCallbackData = data;
}
