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

#ifndef _LAUNCHER_STATUS_BAR_APPLET_H_
#define _LAUNCHER_STATUS_BAR_APPLET_H_

/* StatusbarItem */
#include <libhildondesktop/statusbar-item.h>

/* osso_context_t */
#include <libosso.h>

/* gboolean, gint, G_BEGIN_DECLS/G_END_DECLS */
#include <glib.h>

/* GtkWidget */
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Every plugin has a constant priority */
#define LAUNCHER_STATUS_BAR_APPLET_PRIORITY	1

#define LAUNCHER_STATUS_BAR_APPLET_ICON_SIZE	40

typedef struct _LauncherStatusBarApplet LauncherStatusBarApplet;
typedef struct _LauncherStatusBarAppletClass 
LauncherStatusBarAppletClass;

#define LAUNCHER_TYPE_STATUS_BAR_APPLET            (launcher_status_bar_applet_get_type ())
#define LAUNCHER_STATUS_BAR_APPLET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DUMMY_TYPE_STATUS_BAR_APPLET, LauncherStatusBarApplet))
#define LAUNCHER_STATUS_BAR_APPLET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  DUMMY_TYPE_STATUS_BAR_APPLET, LauncherStatusBarAppletClass))
#define LAUNCHER_IS_STATUS_BAR_APPLET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DUMMY_TYPE_STATUS_BAR_APPLET))
#define LAUNCHER_IS_STATUS_BAR_APPLET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  DUMMY_TYPE_STATUS_BAR_APPLET))
#define LAUNCHER_STATUS_BAR_APPLET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  DUMMY_TYPE_STATUS_BAR_APPLET, LauncherStatusBarAppletClass))

struct _LauncherStatusBarApplet
{
    StatusbarItem parent;
};

struct _LauncherStatusBarAppletClass
{
    StatusbarItemClass parent_class;
};

GType launcher_status_bar_applet_get_type(void);

typedef struct
{
    osso_context_t       *osso;		/* osso */
    GtkWidget            *icon;		/* icon in button */
    GtkWidget	         *button;	/* button in StatusBar */
} LauncherStatusBarAppletPrivate;

G_END_DECLS

#endif /* _LAUNCHER_STATUS_BAR_APPLET_H_ */

