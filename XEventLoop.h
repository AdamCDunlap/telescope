#ifndef __TELESCOPE_XEVENTLOOP_H
#define __TELESCOPE_XEVENTLOOP_H

#include <X11/Xlib.h>

#include <dbus/dbus.h>

#include "LinkedList.h"
#include "Delegate.h"

class XEventHandler;
class XIdleTask;


class Timeout;

typedef Delegate1<Timeout*> TimeoutCallback;

struct Timeout
{
    private:
        struct timeval _absTime;
        TimeoutCallback _callback;

    public:
        Timeout();
        Timeout(struct timeval absTime, TimeoutCallback callback);

        const struct timeval* absTime() const { return &_absTime; }
        TimeoutCallback callback() const { return _callback; }
};

class XEventLoop
{
    private:
        static XEventLoop *_instance;

        Display *_dpy;

        bool _breakEventLoop;

        LinkedList<XEventHandler*> _eventHandlers;

        LinkedList<XIdleTask*> _idleTasks;

        LinkedList<Timeout*> _timeouts;


        LinkedList<DBusWatch*> _dbusWatches;


        static dbus_bool_t dbusAddWatch(DBusWatch *watch, void *data);
        static void dbusRemoveWatch(DBusWatch *watch, void *data);

    public:
        static XEventLoop* instance() { return _instance; }

        XEventLoop(Display *dpy);
        ~XEventLoop();

        void eventLoop();

        void addHandler(XEventHandler *handler);
        void addIdleTask(XIdleTask *idleTask);

        Timeout* addTimeout(float sec, TimeoutCallback callback);
        void cancelTimeout(Timeout* timeout);


        void addDBusConnection(DBusConnection* dbus);
};


#endif
