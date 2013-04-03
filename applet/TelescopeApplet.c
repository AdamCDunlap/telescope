//
// Telescope Applet - status bar applet for calling Telescope task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: TelescopeApplet.c 156 2010-10-14 10:24:05Z mitrandir $

#include "TelescopeApplet.h"

#include <stdio.h>

#include <dbus/dbus.h>

#include <libhildondesktop/libhildondesktop.h>

void mylog(const char *msg)
{
#ifdef ENABLE_LOG
    FILE *f = fopen("/tmp/telescope-applet.log", "a");
    fprintf(f, "%s\n", msg);
    fclose(f);
#endif
}

#define TELESCOPE_APPLET_GET_PRIVATE(x) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((x), TELESCOPE_APPLET_TYPE, TelescopeAppletPriv))

HD_DEFINE_PLUGIN (TelescopeApplet,
                  telescope_applet,
                  STATUSBAR_TYPE_ITEM);


struct _TelescopeAppletPriv
{
    GtkEventBox *event_box;
    GdkPixbuf *icon;

    DBusConnection *conn;
};

void telescope_applet_dispose(GObject *self);

gboolean telescope_applet_on_press(GtkWidget *widget, GdkEventButton *event, TelescopeApplet *self);
gboolean telescope_applet_on_expose(GtkWidget *widget, GdkEventExpose *event, TelescopeApplet *self);


void telescope_applet_class_init (TelescopeAppletClass *klass)
{
mylog("d");
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = telescope_applet_dispose;
//    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    g_type_class_add_private(klass, sizeof(TelescopeAppletPriv));
}


void telescope_applet_init(TelescopeApplet *self)
{
mylog("c");
    self->priv = TELESCOPE_APPLET_GET_PRIVATE (self);

    self->priv->event_box = GTK_EVENT_BOX(gtk_event_box_new());
    self->priv->icon = gdk_pixbuf_new_from_file("/usr/share/telescope-applet/telescope-applet.png", 0);

    gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(self->priv->event_box));
    g_signal_connect(G_OBJECT(self->priv->event_box), "button-press-event", G_CALLBACK(telescope_applet_on_press), self);
    g_signal_connect(G_OBJECT(self), "expose-event", G_CALLBACK(telescope_applet_on_expose), self);
    gtk_widget_set_app_paintable(GTK_WIDGET(self->priv->event_box), TRUE);
    gtk_event_box_set_visible_window(self->priv->event_box, FALSE);

    gtk_widget_show_all(GTK_WIDGET(self));

    DBusError error;
    dbus_error_init(&error);

    self->priv->conn = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (dbus_error_is_set(&error) || self->priv->conn == 0)
    {
      fprintf(stderr, "D-Bus connection error: %s\n", error.message);
      return;
    }
}

void telescope_applet_dispose(GObject *gobject)
{
    TelescopeApplet *self = TELESCOPE_APPLET(gobject);


    if (self->priv->icon)
    {
        g_object_unref(self->priv->icon);
        self->priv->icon = 0;
    }


    if (self->priv->conn)
    {
      dbus_connection_unref(self->priv->conn);

      self->priv->conn = 0;
    }

    G_OBJECT_CLASS(g_type_class_peek_parent(G_OBJECT_GET_CLASS(gobject)))->dispose(gobject);
}


gboolean telescope_applet_on_press(GtkWidget *widget, GdkEventButton *event, TelescopeApplet *self)
{
mylog("a");
    DBusMessage *call = dbus_message_new_method_call(
      "org.telescope",
      "/Telescope",
      "org.telescope.Telescope",
      "Show"
    );

    dbus_connection_send(self->priv->conn, call, 0);
    dbus_connection_flush(self->priv->conn);

    dbus_message_unref(call);

    return TRUE;
}


gboolean telescope_applet_on_expose(GtkWidget *widget, GdkEventExpose *event, TelescopeApplet *self)
{
    GdkGC *gc = gdk_gc_new(widget->window);

    int x = widget->allocation.x + (widget->allocation.width  - gdk_pixbuf_get_width(self->priv->icon)) / 2;
    int y = widget->allocation.y + (widget->allocation.height - gdk_pixbuf_get_height(self->priv->icon)) / 2;

    gdk_draw_pixbuf(widget->window, gc,
        self->priv->icon,
        0, 0, x, y, -1, -1,
        GDK_RGB_DITHER_NORMAL, 0, 0);

    g_object_unref(gc);

    return TRUE;
}
