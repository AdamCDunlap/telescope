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

#ifndef CONSTANT_H
#define CONSTANT_H

#define NUM_ROWS    3
#define NUM_COLS    5

#ifdef DEV

#define APPLICATION_MENU        "applications.menu"
#define BACKGROUND_CONF         "home-background.conf"
#define DESKTOP_FILE_PATH       "./hildon/"
#define ICON_PATH               "./scalable/"
#define SHARE_PATH              ""
#else

#define APPLICATION_MENU        "/home/user/.osso/menus/applications.menu"
#define APPLICATION_MENU_STOCK  "/etc/xdg/menus/applications.menu"
#define BACKGROUND_CONF         "/home/user/.osso/hildon-desktop/home-background.conf"
#define DESKTOP_FILE_PATH       "/usr/share/applications/hildon/"
#define ICON_PATH               "/usr/share/icons/hicolor/"
#define SHARE_PATH              "/usr/share/telescope/"

#endif // DEV

#endif // CONSTANT_H
