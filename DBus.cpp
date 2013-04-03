//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: DBus.cpp 174 2011-02-28 17:17:05Z mitrandir $

#ifdef DBUS

#include "DBus.h"

#include "XEventLoop.h"
#include "TeleWindow.h"
#include "LauncherWindow.h"

DBus * DBus::_instance = 0;


#ifdef LAUNCHER
DBus::DBus(XEventLoop *eventLoop, TeleWindow *teleWindow, LauncherWindow *launcherWindow)
#else
DBus::DBus(XEventLoop *eventLoop, TeleWindow *teleWindow)
#endif
{
    _instance = this;

    _teleWindow = teleWindow;

    #ifdef LAUNCHER
        _launcherWindow = launcherWindow;
    #endif


    DBusError error;
    dbus_error_init(&error);

    _conn = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (dbus_error_is_set(&error) || _conn == 0)
    {
        fprintf(stderr, "D-Bus connection error: %s\n", error.message);
        return;
    }

    _dispatchOnIdle = true;
    dbus_connection_set_dispatch_status_function(
        _conn,
        dispatchStatusChanged,
        this,
        0
    );

    dbus_bus_request_name(_conn, "org.telescope",
            DBUS_NAME_FLAG_DO_NOT_QUEUE, &error);
    if (dbus_error_is_set(&error))
    {
        fprintf(stderr, "D-Bus bus name error: %s\n", error.message);
        return;
    }


    _telescopeVTable.unregister_function = (DBusObjectPathUnregisterFunction)telescope_unregister;
    _telescopeVTable.message_function = (DBusObjectPathMessageFunction)telescope_message;
    dbus_connection_register_object_path(_conn, "/Telescope",
        &_telescopeVTable, this);


    #ifdef LAUNCHER
        _launcherVTable.unregister_function = (DBusObjectPathUnregisterFunction)launcher_unregister;
        _launcherVTable.message_function = (DBusObjectPathMessageFunction)launcher_message;
        dbus_connection_register_object_path(_conn, "/Launcher",
            &_launcherVTable, this);
    #endif



    eventLoop->addIdleTask(this);
    eventLoop->addDBusConnection(_conn);
}

DBus::~DBus()
{
    dbus_connection_unref(_conn);
}


void DBus::dispatchStatusChanged(DBusConnection *conn, DBusDispatchStatus status, void *data)
{
    if (status == DBUS_DISPATCH_DATA_REMAINS)
        static_cast<DBus*>(data)->_dispatchOnIdle = true;
}


void DBus::onIdle()
{
    if (_dispatchOnIdle)
    {
        _dispatchOnIdle = false;

        while (dbus_connection_get_dispatch_status(_conn) == DBUS_DISPATCH_DATA_REMAINS)
            dbus_connection_dispatch(_conn);
    }
}







void DBus::telescope_unregister(DBusConnection *conn, DBus *self)
{
}

DBusHandlerResult DBus::telescope_message(
    DBusConnection *conn,
    DBusMessage *msg,
    DBus *self
)
{
    if (dbus_message_is_method_call(msg, "org.telescope.Telescope", "Show"))
    {
        DBusMessage *reply = dbus_message_new_method_return(msg);
        dbus_connection_send(conn, reply, 0);
        dbus_message_unref(reply);

        self->_teleWindow->show();
    }
    else if (dbus_message_is_method_call(msg, "org.telescope.Telescope", "Hide"))
    {
        DBusMessage *reply = dbus_message_new_method_return(msg);
        dbus_connection_send(conn, reply, 0);
        dbus_message_unref(reply);

        self->_teleWindow->hide();
    }

    XFlush(self->_teleWindow->display());

    return DBUS_HANDLER_RESULT_HANDLED;
}


#ifdef LAUNCHER

void DBus::launcher_unregister(DBusConnection *conn, DBus *self)
{
}

DBusHandlerResult DBus::launcher_message(
    DBusConnection *conn,
    DBusMessage *msg,
    DBus *self
)
{
    if (dbus_message_is_method_call(msg, "org.telescope.Launcher", "Show"))
    {
        DBusMessage *reply = dbus_message_new_method_return(msg);
        dbus_connection_send(conn, reply, 0);
        dbus_message_unref(reply);

        self->_launcherWindow->show();
    }
    else if (dbus_message_is_method_call(msg, "org.telescope.Launcher", "Hide"))
    {
        DBusMessage *reply = dbus_message_new_method_return(msg);
        dbus_connection_send(conn, reply, 0);
        dbus_message_unref(reply);

        self->_launcherWindow->hide();
    }

    XFlush(self->_launcherWindow->display());

    return DBUS_HANDLER_RESULT_HANDLED;
}

#endif


#endif
