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

#include <stdlib.h>

#include <libintl.h>
#include <locale.h>

#include <Imlib2.h>

#include "Application.h"
#include "Resources.h"
#include "constant.h"
#include "XTools.h"
#include "DBus.h"

#include "Image.h"

int Application::_pixmapWidth = 104;
int Application::_pixmapHeight = 96;
XftFont * Application::_xftFont = 0;

Application::Application(Display *dpy, const gchar *filename)
{
    _dpy = dpy;
    _filename = 0;
    _icon = 0;
    _iconPath = 0;
    _executable = 0;
    _service = 0;
    _image = 0;

    GKeyFile* keyFile = g_key_file_new();
    gchar *group;
    gchar *displayed;

    _filename = g_strconcat ( DESKTOP_FILE_PATH, filename, NULL );

    if ( g_key_file_load_from_file ( keyFile, _filename, G_KEY_FILE_KEEP_COMMENTS, NULL ) )
    {
        _isValid = TRUE;

        group = g_key_file_get_start_group ( keyFile );

        displayed = g_key_file_get_string ( keyFile, group, "NoDisplay", NULL );
        if ( displayed != NULL && g_ascii_strncasecmp ( displayed, "true", strlen( displayed ) ) == 0 )
            _isValid = FALSE;

        g_free( displayed );

        if ( _isValid )
        {
            // get icon path and filename
            _iconPath = g_key_file_get_string ( keyFile, group, "X-Icon-path", NULL);
            _icon = g_strconcat(g_key_file_get_string ( keyFile, group, "Icon", NULL), ".png", NULL);
            bool useDefaultIcon = TRUE;
            if (_icon != NULL)
        {
                if ( g_file_test ( g_strconcat ( ICON_PATH, "scalable/hildon/", _icon, NULL ), G_FILE_TEST_EXISTS ) )
                {
                    useDefaultIcon = FALSE;
                    _iconPath = g_strconcat ( ICON_PATH, "scalable/hildon/", NULL );
                }
                else if ( _iconPath != NULL && g_file_test ( g_strconcat ( _iconPath, _icon, NULL ), G_FILE_TEST_EXISTS ) )
                {
                    useDefaultIcon = FALSE;
                }
                else if ( g_file_test ( g_strconcat ( ICON_PATH, "64x64/apps/", _icon, NULL ), G_FILE_TEST_EXISTS ) )
                {
                    useDefaultIcon = FALSE;
                    _iconPath = g_strconcat ( ICON_PATH, "64x64/apps/", NULL );
                }
                else if ( g_file_test ( g_strconcat ( ICON_PATH, "64x64/hildon/", _icon, NULL ), G_FILE_TEST_EXISTS ) )
                {
                    useDefaultIcon = FALSE;
                    _iconPath = g_strconcat ( ICON_PATH, "64x64/hildon/", NULL );
                }
                else if ( g_file_test ( g_strconcat ( ICON_PATH, "scalable/apps/" , _icon, NULL), G_FILE_TEST_EXISTS ) )
                {
                    useDefaultIcon = FALSE;
                    _iconPath = g_strconcat ( ICON_PATH, "scalable/apps/", NULL );
                }
                else if ( g_file_test ( g_strconcat ( "/usr/share/pixmaps/", _icon, NULL), G_FILE_TEST_EXISTS ) )
                {
                    useDefaultIcon = FALSE;
                    _iconPath = g_strdup ( "/usr/share/pixmaps/" );
                }
            }

            if ( useDefaultIcon )
            {
                _iconPath = g_strconcat ( ICON_PATH, "scalable/hildon/", NULL );
                _icon = g_strdup ( "qgn_list_gene_default_app.png" );
            }

            // get the application name;
            _appName = gettext( g_key_file_get_string ( keyFile, group, "Name", NULL ) );

            _executable = g_key_file_get_string ( keyFile, group, "Exec", NULL );

            gchar * terminal =  g_key_file_get_string ( keyFile, group, "Terminal", NULL );
            _runInTerminal = FALSE;
            if ( terminal != NULL && g_ascii_strncasecmp ( terminal, "true", 5 ) == 0 )
            {
               _runInTerminal = TRUE;
            }
            g_free(terminal);

            // get the dbus service
            _service = g_key_file_get_string( keyFile, group, "X-Osso-Service", NULL );
            if ( _service != NULL && g_strstr_len ( _service, strlen ( _service ), "." ) == NULL )
            {
                _service = g_strconcat ( "com.nokia.", _service, NULL );
            }
        }
        g_free ( group );
    }
    else
    {
        printf("Cannot load %s file\n", _filename);
        _isValid = FALSE;
    }

    g_key_file_free ( keyFile );

    if (_isValid)
        createPicture();
}

Application::~Application()
{
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Application dtor");

    if ( _filename != NULL )
        g_free ( _filename );

    if ( _icon != NULL )
        g_free ( _icon );

    if ( _iconPath != NULL )
        g_free ( _iconPath );

    if ( _executable != NULL )
        g_free ( _executable );

    if ( _service != NULL )
        g_free ( _service );

    if ( _xftFont )
    {
        XftFontClose( _dpy, _xftFont );
        _xftFont = 0;
    }
}

bool Application::isAHit(int x, int y)
{
     if ( x >= _x && x <= _x + _pixmapWidth && y >= _y && y <= _y + _pixmapHeight)
         return TRUE;
     return FALSE;
}

const gchar* Application::getIcon()
{
    return _icon;
}

const gchar* Application::getApplicationName()
{
    return _appName;
}

bool Application::execute()
{
    printf("executing %s\n", _appName);
    if ( _runInTerminal )
        return _executeInTerminal();

    // try to execute the application through DBus service first
    if ( _service != NULL )
    {
        if ( ! _executeService() )
           return _executeNormally();
        else
           return TRUE;
    }
    else
      return _executeNormally();

    return FALSE;
}

bool Application::_executeNormally()
{
    printf("executing normally: %s\n", _executable);
    gint argc;
    gchar** argv;

    if ( g_shell_parse_argv ( _executable, &argc, &argv, NULL ) )
    {
        return g_spawn_async ( NULL, argv, NULL, ( GSpawnFlags ) G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL );
    }
    return FALSE;
}

bool Application::_executeService()
{
    printf("Executing by DBus service: %s\n", _service);

    DBusConnection *conn = DBus::instance()->getConnection();
    
    DBusMessage *message = dbus_message_new_method_call(
        _service,
        "/",
        _service,
        "top_application"
    );

    bool success = dbus_connection_send(conn, message, NULL);
    dbus_message_unref(message);

    return success;
}

bool Application::_executeInTerminal()
{
    printf("Executing in terminal\n");

    DBusMessage *call = dbus_message_new_method_call(
        "com.nokia.xterm",
        "/com/nokia/xterm",
        "com.nokia.xterm",
        "run_command"
    );

    gchar * executable = g_strconcat("sh -c \"", g_strescape( _executable, NULL), " ; read x\"", NULL);
    dbus_message_append_args (call,
                              DBUS_TYPE_STRING, executable,
                              DBUS_TYPE_INVALID);

    bool success = dbus_connection_send(DBus::instance()->getConnection(), call, 0);

    dbus_message_unref(call);
    g_free ( executable );

    return success;
}

bool Application::isValid()
{
    return _isValid;
}

void Application::setPosition ( int x, int y )
{
    _x = x;
    _y = y;
}

//Picture Application::draw(Display *dpy)
void Application::createPicture()
{
    _image = new Image(_dpy, _pixmapWidth, _pixmapHeight);
    _image->clear();

    Resources * resources = Resources::instance();

    // draw a transparent canvas

    // Load icon and draw icon

    gchar *filename = g_strconcat(_iconPath, _icon, NULL);
    Image* icon = new Image(_dpy, filename);
    g_free(filename);

    if (! icon->valid())
        printf ( "Cannot load icon: %s%s\n", _iconPath, _icon );
    else
    {
        XRenderComposite(
            _dpy, PictOpSrc,
            icon->picture(), None, _image->picture(),
            0, 0,
            0, 0,
            (_image->width() - icon->width()) / 2, // x pos
            (64 - icon->width()) / 2, // y pos
            icon->width(), // width
            icon->height() // height
        );
    }

    // Load and draw text background
    XRenderComposite(
        _dpy, PictOpSrc,
        resources->getTextBackground()->picture(), None, _image->picture(),
        0, 0,
        0, 0,
        0, // x pos
        _pixmapHeight - resources->getTextBackground()->height(), // y pos
        resources->getTextBackground()->width(), // width
        resources->getTextBackground()->height() // height
    );

    // Drawing application text
    XftDraw* xftDraw = XftDrawCreate(
        _dpy, _image->pixmap(),
        XTools::rgbaVisual()->visual,
        DefaultColormap(_dpy, DefaultScreen(_dpy))
    );

    if (_xftFont == 0)
    {
        _xftFont = XftFontOpen(_dpy, DefaultScreen(_dpy),
            XFT_FAMILY, XftTypeString, "sans",
            XFT_PIXEL_SIZE, XftTypeInteger, 15,
            NULL
        );
    }

    XGlyphInfo textInfo;
    XftTextExtentsUtf8(_dpy, _xftFont, (const FcChar8*)_appName, strlen(_appName), &textInfo);

    XftColor fontColor;
    fontColor.pixel = 0;
    fontColor.color.red     = 0xffff;
    fontColor.color.green   = 0xffff;
    fontColor.color.blue    = 0xffff;
    fontColor.color.alpha   = 0xffff;

    XRectangle rect =
    {
        2,
        _pixmapHeight - resources->getTextBackground()->height() + 2,
        resources->getTextBackground()->width() - 4,
        resources->getTextBackground()->height() - 4
    };
    Region clip = XCreateRegion();
    XUnionRectWithRegion ( &rect, clip, clip );

    XftDrawSetClip ( xftDraw, clip );

    int textXPos = textInfo.width > ( resources->getTextBackground()->width() - 4 ) ? 2 : ( resources->getTextBackground()->width() - 4 - textInfo.width ) / 2;
    XftDrawStringUtf8 ( xftDraw, &fontColor, _xftFont,
                        textXPos,
                         _pixmapHeight - resources->getTextBackground()->height() + 15 + 2,
                        ( const FcChar8* ) _appName,
                        strlen ( _appName )
                      );

    XDestroyRegion ( clip );

    // free resources
    delete icon;

    XftDrawDestroy ( xftDraw );
}

