//
// Telescope Applet - status bar applet for calling Telescope task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: TelescopeApplet.h 110 2010-07-08 08:32:28Z mitrandir $

#ifndef __TELESCOPE_APPLET_H
#define __TELESCOPE_APPLET_H

#include <glib-object.h>
#include <libhildondesktop/statusbar-item.h>

G_BEGIN_DECLS

/* Common struct types declarations */
typedef struct _TelescopeApplet      TelescopeApplet;
typedef struct _TelescopeAppletClass TelescopeAppletClass;
typedef struct _TelescopeAppletPriv  TelescopeAppletPriv;

/* Common macros */
#define TELESCOPE_APPLET_TYPE            (telescope_applet_get_type ())
#define TELESCOPE_APPLET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TELESCOPE_APPLET_TYPE, TelescopeApplet))
#define TELESCOPE_APPLET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  TELESCOPE_APPLET_TYPE, TelescopeAppletClass))
#define IS_TELESCOPE_APPLET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TELESCOPE_APPLET_TYPE))
#define IS_TELESCOPE_APPLET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  TELESCOPE_APPLET_TYPE))
#define TELESCOPE_APPLET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  TELESCOPE_APPLET_TYPE, TelescopeAppletClass))

/* Instance struct */
struct _TelescopeApplet
{
    StatusbarItem parent;

    TelescopeAppletPriv *priv;
};

/* Class struct */
struct _TelescopeAppletClass
{
    StatusbarItemClass parent_class;
};

GType  telescope_applet_get_type  (void);

G_END_DECLS

#endif

