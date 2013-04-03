#include "XEventLoop.h"

#include <math.h>
#include <sys/time.h>

#include "XEventHandler.h"
#include "XIdleTask.h"


#ifdef MAEMO4
    #define dbus_watch_get_unix_fd    dbus_watch_get_fd
#endif


XEventLoop* XEventLoop::_instance = 0;


XEventLoop::XEventLoop(Display *dpy)
{
    XEventLoop::_instance = this;

    _dpy = dpy;

    _breakEventLoop = false;
}

XEventLoop::~XEventLoop()
{
}


void XEventLoop::addHandler(XEventHandler *handler)
{
    _eventHandlers.append(handler);
}


void XEventLoop::addIdleTask(XIdleTask *idleTask)
{
    _idleTasks.append(idleTask);
}


void XEventLoop::eventLoop()
{
    XFlush(_dpy);

    int xSocket = XConnectionNumber(_dpy);
    fd_set fdset;

    Timeout* nearestTimeout = 0;
    struct timeval remaining;

    _breakEventLoop = false;
    while (! _breakEventLoop)
    {
        if (_timeouts.size() > 0)
        {
            nearestTimeout = 0;

            while (_timeouts.size() > 0 && nearestTimeout == 0)
            {
                nearestTimeout = *_timeouts.head();
                remaining = *nearestTimeout->absTime();
                struct timeval cur;
                gettimeofday(&cur, 0);

                if (timercmp(&cur, &remaining, >))
                {
                    // already should be fired
                    nearestTimeout->callback()(nearestTimeout);
                    _timeouts.remove(0);
                    continue;
                }

                timersub(&remaining, &cur, &remaining);
            }
        }
        else
            nearestTimeout = 0;


        FD_ZERO(&fdset);
        FD_SET(xSocket, &fdset);
        int maxSocket = xSocket;

        for (LinkedList<DBusWatch*>::Iter i = _dbusWatches.head(); i; ++i)
            if (dbus_watch_get_enabled(*i))
            {
                int dbusSocket = dbus_watch_get_unix_fd(*i);
                FD_SET(dbusSocket, &fdset);
                if (dbusSocket > maxSocket)
                    maxSocket = dbusSocket;
            }

        if (select(maxSocket+1, &fdset, 0, 0, nearestTimeout ? &remaining : 0))
        {
            for (LinkedList<DBusWatch*>::Iter i = _dbusWatches.head(); i; ++i)
                if (FD_ISSET(dbus_watch_get_unix_fd(*i), &fdset))
                    dbus_watch_handle(*i, DBUS_WATCH_READABLE | DBUS_WATCH_WRITABLE);
        }
        else
            if (nearestTimeout)
            {
                _timeouts.remove(0);
                nearestTimeout->callback()(nearestTimeout);
                nearestTimeout = 0;
            }



        for (LinkedList<XIdleTask*>::Iter i = _idleTasks.head(); i; ++i)
            (*i)->onIdle();

        XSync(_dpy, False);

        while (XPending(_dpy))
        {
            XEvent event;
            XNextEvent(_dpy, &event);

            for (LinkedList<XEventHandler*>::Iter i = _eventHandlers.head(); i; ++i)
                (*i)->onEvent(&event);

            for (LinkedList<XIdleTask*>::Iter i = _idleTasks.head(); i; ++i)
                (*i)->onIdle();

            XSync(_dpy, False);
        }
    }
}





void addToTimeval(float sec, struct timeval *tv)
{
    double intpart;
    tv->tv_usec += int(modf(sec, &intpart) * 1000000);
    tv->tv_sec += (time_t)intpart;
    if (tv->tv_usec > 1000000)
        tv->tv_sec++;
}


Timeout* XEventLoop::addTimeout(float sec, TimeoutCallback callback)
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    addToTimeval(sec, &tv);

    Timeout* timeout = new Timeout(tv, callback);

    LinkedList<Timeout*>::Iter i = _timeouts.head();
    int n = 0;
    while (i)
    {
        if (timercmp((*i)->absTime(), &tv, >))
            break;

        i++; n++;
    }

    _timeouts.insert(n, timeout);

    return timeout;
}

void XEventLoop::cancelTimeout(Timeout* timeout)
{
    _timeouts.removeByValue(timeout);
}



Timeout::Timeout()
    :_callback(0)
{
    _absTime.tv_sec = 0;
    _absTime.tv_usec= 0;
}

Timeout::Timeout(struct timeval absTime, TimeoutCallback callback)
    :_absTime(absTime), _callback(callback)
{
}




void XEventLoop::addDBusConnection(DBusConnection *dbus)
{
    dbus_connection_set_watch_functions(dbus,
        dbusAddWatch,
        dbusRemoveWatch,
        0,
        this,
        0
    );
}



dbus_bool_t XEventLoop::dbusAddWatch(DBusWatch *watch, void *data)
{
    static_cast<XEventLoop*>(data)->_dbusWatches.append(watch);
    return true;
}

void XEventLoop::dbusRemoveWatch(DBusWatch *watch, void *data)
{
    static_cast<XEventLoop*>(data)->_dbusWatches.removeByValue(watch);
}
