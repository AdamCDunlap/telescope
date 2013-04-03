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

#ifndef SECTION_H
#define SECTION_H

#include <glib.h>
#include "Application.h"

class Image;


class Section
{
public:
    Section(Display *dpy, const gchar *name);
    ~Section();

    void setPart(guint partNo);
    guint getPart();
    void setName(const gchar *name);
    const gchar * getName();
    gchar * getNameWithPart();
    Application * getApplication(guint index);
    void addApplication(Application *app);
    guint getApplicationsSize();
    void draw(Display *dpy, Image* image, int x, int y, int width, int height);

    Image* getIcon() { return _icon; }
    bool isIconOwned() { return _iconOwned; }
    void setIcon(Image *icon, bool own);

    typedef void (*IconChangedCallback)(Section *section, void *data);
    void setIconChangedCallback(IconChangedCallback callback, void *data);

private:
    gchar* _name;
    guint _partNo;
    GPtrArray* _applications;

    Display * _dpy;
    static XftFont *_xftFont;

    Image* _icon;
    bool _iconOwned;

    IconChangedCallback _iconChangedCallback;
    void *_iconChangedCallbackData;
};

#endif // SECTION_H

