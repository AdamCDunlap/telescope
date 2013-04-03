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

/* Hildon includes */
#include <hildon/hildon-note.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-sound.h>
#include <hildon/hildon-defines.h>
#include <libhildondesktop/libhildondesktop.h>

#include <log-functions.h>
#include <libosso.h>
#include <osso-log.h>

/* GTK includes */
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkpixbuf.h>

/* Systems includes */
#include <string.h>


#include "LauncherTaskNavigatorApplet.h"

HD_DEFINE_PLUGIN(LauncherNavigatorApplet, launcher_navigator_applet, TASKNAVIGATOR_TYPE_ITEM);

#define LAUNCHER_NAVIGATOR_APPLET_GET_PRIVATE(x)      (G_TYPE_INSTANCE_GET_PRIVATE((x), launcher_navigator_applet_get_type(), LauncherNavigatorAppletPrivate));

static gboolean applet_icon_pressed(GtkWidget *widget, GdkEventButton *button, gpointer data);
static void launcher_navigator_applet_finalize(GObject *object);

static void launcher_navigator_applet_class_init(LauncherNavigatorAppletClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = launcher_navigator_applet_finalize;
    g_type_class_add_private(klass, sizeof(LauncherNavigatorAppletPrivate));
}

static void set_applet_icon(const gchar *name, LauncherNavigatorAppletPrivate *info)
{
    GtkIconTheme *icon_theme;
    GdkPixbuf    *pixbuf;

    icon_theme = gtk_icon_theme_get_default();

    pixbuf = (name != NULL) ? gtk_icon_theme_load_icon(icon_theme, name,
                                                       LAUNCHER_NAVIGATOR_APPLET_ICON_SIZE,
                                                       GTK_ICON_LOOKUP_NO_SVG, NULL) : NULL;

    gtk_image_set_from_pixbuf(GTK_IMAGE(info->icon), pixbuf);

    if (pixbuf != NULL)
        g_object_unref(pixbuf);
}

/**
 * Callback for the button clicked signal
 *
 * @param widget the button widget
 * @param data applet info struct
 */
static gboolean applet_icon_pressed(GtkWidget *widget, GdkEventButton *button, gpointer data)
{
    LauncherNavigatorAppletPrivate *info;

    g_return_val_if_fail(data, FALSE);
    info = (LauncherNavigatorAppletPrivate*)data;

    gtk_button_released(GTK_BUTTON(info->button));

    osso_rpc_t retval;
    osso_return_t result;
    
    result = osso_rpc_run(info->osso,
                 "org.telescope",
                 "/Launcher",
                 "org.telescope.Launcher",
                 "Show", &retval, DBUS_TYPE_INVALID);

    osso_rpc_free_val(&retval);

    (void) button;

    return TRUE;
}

static void launcher_navigator_applet_finalize(GObject *object)
{
    LauncherNavigatorAppletPrivate *info = LAUNCHER_NAVIGATOR_APPLET_GET_PRIVATE(object);

    osso_deinitialize(info->osso);

    LOG_CLOSE();

    G_OBJECT_CLASS(g_type_class_peek_parent(G_OBJECT_GET_CLASS(object)))->finalize(object);
}

static void launcher_navigator_applet_init(LauncherNavigatorApplet *applet)
{
    LauncherNavigatorAppletPrivate *info = LAUNCHER_NAVIGATOR_APPLET_GET_PRIVATE(applet);

    ULOG_OPEN("launcher-statusbar-applet");

    g_return_if_fail(info);

    info->icon = gtk_image_new_from_pixbuf(NULL);
    info->button = gtk_toggle_button_new();

    set_applet_icon("qgn_grid_tasknavigator_others", info);
    
    gtk_widget_set_name (info->button, "hildon-navigator-button-one");

    gtk_container_add(GTK_CONTAINER(info->button),
                      GTK_WIDGET(info->icon));

    gtk_container_add(GTK_CONTAINER(applet), info->button);

    /* Signal for icon (button) */
    g_signal_connect(G_OBJECT(info->button), "button-press-event",
                     G_CALLBACK(applet_icon_pressed), info);

    /* Initialize osso */
    info->osso = osso_initialize("org.telescope.LauncherApplet", "0.1", TRUE, NULL);
    if (!info->osso)
        ULOG_WARN("%s: error while initializing osso\n", __FUNCTION__);

    gtk_widget_show_all(GTK_WIDGET(applet));
}
