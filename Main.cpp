//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: Main.cpp 184 2011-06-28 19:51:40Z mitrandir $
#include <libintl.h>
#include <locale.h>

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>

#include "TeleWindow.h"
#include "XTools.h"
#include "Settings.h"
#include "Resources.h"
#include "DBus.h"

#include "XEventLoop.h"


#ifdef LAUNCHER
    #include "LauncherWindow.h"
    #include "SectionList.h"
    #include "MenuReader.h"
#endif



// We need to catch SIGCHLD in order to not leave zombie processes that was runned by execvp
#include <sys/types.h>
#include <sys/wait.h>

void sig_chld(int signo)
{
    int status;
    waitpid(-1, &status, WNOHANG);
}



int main(int argc, char *argv[])
{
    // Add handling for SIGCHLD
    struct sigaction act;
    act.sa_handler = sig_chld;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_NOCLDSTOP;
    sigaction(SIGCHLD, &act, 0);


    // prepare i8n
    setlocale(LC_ALL,"");
    bindtextdomain("maemo-af-desktop","/usr/share/locale");
    textdomain("maemo-af-desktop");

    Display *dpy = XOpenDisplay(0);

    if (! dpy)
    {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    XTools::init(dpy);

    if (! XTools::checkCompositeExtension())
    {
        fprintf(stderr, "XServer doesn't support Composite\n");
        return 1;
    }

    if (! XTools::checkDamageExtension())
    {
        fprintf(stderr, "XServer doesn't support Damage\n");
        return 1;
    }

    if (! XTools::checkXRenderExtension())
    {
        fprintf(stderr, "XServer doesn't support XRender\n");
        return 1;
    }

    XTools::enableCompositeRedirect();



    Settings *settings = new Settings;

    // init resource
    Resources * resources = new Resources(dpy);


    XEventLoop *eventLoop = new XEventLoop(dpy);


    TeleWindow *teleWindow = new TeleWindow(dpy);

    #ifdef LAUNCHER
        MenuReader *menuReader = new MenuReader(dpy);
        LauncherWindow *launcherWindow = 0;
        if (! settings->disableLauncher())
            launcherWindow = new LauncherWindow(dpy/*, list*/);
    #endif

    #ifdef LAUNCHER
        DBus *dbus = new DBus(eventLoop, teleWindow, launcherWindow);
    #else
        DBus *dbus = new DBus(eventLoop, teleWindow);
    #endif


    eventLoop->eventLoop();

    delete eventLoop;

    delete dbus;

    delete teleWindow;

    #ifdef LAUNCHER
        delete launcherWindow;
        delete menuReader;
    #endif

    delete resources;
    delete settings;

    XCloseDisplay(dpy);
}
