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

#include <string.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <dirent.h>

#include <glib.h>

#include "MenuReader.h"
#include "constant.h"

#include "Image.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

gboolean newSection = FALSE;
gboolean newApplication = FALSE;

void start_element (GMarkupParseContext *context,
                    const gchar         *element_name,
                    const gchar        **attribute_names,
                    const gchar        **attribute_values,
                    gpointer             user_data,
                    GError             **error)
{
    if (strcmp(element_name, "Name") == 0)
    {
        newSection = TRUE;
        newApplication = FALSE;
    }
    else if (strcmp(element_name, "Filename") == 0)
    {
        newSection = FALSE;
        newApplication = TRUE;
    }
    else if (strcmp(element_name, "All") == 0)
    {
        MenuReader::getInstance()->setCurrentSectionAsCatchAll();
    }
    else
    {
        newSection = FALSE;
        newApplication = FALSE;
    }
}

void end_element(GMarkupParseContext *context,
                 const gchar         *element_name,
                 gpointer             user_data,
                 GError             **error)
{
    if (strcmp(element_name, "Name") == 0)
    {
        newSection = FALSE;
        newApplication = FALSE;
    }
}

void text(GMarkupParseContext *context,
          const gchar         *text,
          gsize                text_len,
          gpointer             user_data,
          GError             **error)
{
    if (strncmp(text, "\n", 1) == 0)
        return;

    gchar *name = g_strndup(text, text_len);
    if (newSection && text_len != 0)
    {
        MenuReader::getInstance()->beginSection(name);
    }
    else if (newApplication && text_len != 0)
    {
        MenuReader::getInstance()->addApplication(name);
    }
    g_free(name);
}

static GMarkupParser parser = {
    start_element,
    end_element,
    text,
    NULL,
    NULL
};

void addExtraApplication(gpointer key, gpointer value, gpointer user_data)
{
    printf ("Extra application: %s\n", (char *) value);
    ((MenuReader *) user_data)->addApplication( (gchar *) value, true);
}

MenuReader * MenuReader::_instance = NULL;
//MenuReader * MenuReader::getInstance()
//{
//    if (_instance == NULL)
//        _instance = new MenuReader();
//    return _instance;
//}

MenuReader::MenuReader(Display *dpy)
{
    _instance = this;
    _dpy = dpy;
    _init();

    _fd = inotify_init();
    _applicationMenuWD = -1;
    _desktopsFileWD = -1;

    if ( _fd < 0 )
        printf ( "error inotify_init\n" );
    else
    {
        _applicationMenuWD = inotify_add_watch( _fd, APPLICATION_MENU, IN_MODIFY );      
        _desktopsFileWD = inotify_add_watch( _fd, DESKTOP_FILE_PATH, IN_MOVE | IN_MODIFY | IN_CREATE | IN_DELETE );

        if ( _applicationMenuWD < 0)
          printf ("error inotify_add_watch\n");

    if ( _desktopsFileWD < 0)
          printf ("error watching desktop file path\n");
    }
}

MenuReader::~MenuReader()
{
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "MenuReader dtor");
    _cleanup();

   ( void ) inotify_rm_watch( _fd, _applicationMenuWD );
   ( void ) inotify_rm_watch( _fd, _desktopsFileWD );
   ( void ) close( _fd );
}

void MenuReader::_init()
{
    _currentSection = NULL;
    _extraSection = NULL;
    _originExtraSection = NULL;

    _list = new SectionList();
    _partNo = 1;
    _table = g_hash_table_new ( g_str_hash, g_str_equal );
}

void MenuReader::_cleanup()
{
//     if (_currentSection != NULL)
//     {
//         delete _currentSection;
//         _currentSection = 0;
//     }

    // The user should delete the list
    if (_list != NULL)
    {
        delete _list;
        _list = 0;
    }
}

void MenuReader::beginSection(const gchar *name)
{
    if (_currentSection != NULL && strcmp(_currentSection->getName(), name) == 0)
        ++_partNo;
    else
        _partNo = 1;

    if (_currentSection != NULL)
        endSection();

    _currentSection = new Section(_dpy, name);
    _currentSection->setPart(_partNo);
}

void MenuReader::addApplication(const gchar *desktopFilename, bool isExtra)
{
    if (_currentSection == NULL) {
        return;
    }

    // Create a new section if the limit of icons is reached
    if (_currentSection->getApplicationsSize() >= (NUM_ROWS * NUM_COLS))
    {
        Section * temp = _currentSection;

        beginSection(_currentSection->getName());

        // Set the catch all section to the current section
        if (temp == _extraSection)
            _extraSection = _currentSection;
    }

    Application *app = new Application(_dpy, desktopFilename);

    // remove them from the list of desktop files
    if ( ! isExtra )
        g_hash_table_remove ( _table, desktopFilename );

    if(app->isValid())
        _currentSection->addApplication(app);
    else
        delete app;
}

void MenuReader::setCurrentSectionAsCatchAll()
{
    _originExtraSection = _extraSection = _currentSection;
}

void MenuReader::endSection()
{
    if (_currentSection == NULL)
        return;

    // only add the section to the list if it's not empty
    if ( _currentSection->getApplicationsSize() > 0 )
    {
        if ( ! _list->has( _currentSection ) )
        {
            _list->add(_currentSection);

            // change the original extra section to the succefully added extra section
            if ( _currentSection == _extraSection )
                _originExtraSection = _extraSection;
        }
    }
    else
    {
        if ( _currentSection == _extraSection )
            _extraSection = _originExtraSection;

        _partNo--;

        if ( _partNo < 1 ) _partNo = 1;
    }

    _currentSection = NULL;
}

void MenuReader::_getDesktopFiles()
{
    DIR *dp;
    struct dirent *ep;

    dp = opendir (DESKTOP_FILE_PATH);
    if (dp != NULL)
    {
        while ((ep = readdir (dp)))
        {
            // only insert .desktop file
            if (g_str_has_suffix(ep->d_name, ".desktop"))
                g_hash_table_insert ( _table, g_strdup(ep->d_name), g_strdup ( ep->d_name ) );
        }
        (void) closedir (dp);
    }
    else
        printf("Couldn't open the directory");

}

SectionList * MenuReader::processMenu()
{
    _cleanup();
    _init();

    _getDesktopFiles();

    char *text;
    gsize length;

    printf("Reading application menu\n");
    GMarkupParseContext *context = g_markup_parse_context_new (
      &parser,
      G_MARKUP_TREAT_CDATA_AS_TEXT,
      NULL,
      NULL);

    if (g_file_get_contents (APPLICATION_MENU, &text, &length, NULL) == FALSE) {
        if (g_file_get_contents (APPLICATION_MENU_STOCK, &text, &length, NULL) == FALSE) {
            return NULL;
        }
    }

    if (g_markup_parse_context_parse (context, text, length, NULL) == FALSE || _list->getSize() == 0) {
        // applications.menu file seems to be broken or empty

        printf("user's applications.menu seems to be empty or broken, falling back to stock one\n");

        _cleanup();
        _init();

        if (g_file_get_contents(APPLICATION_MENU_STOCK, &text, &length, NULL) == FALSE)
            return NULL;

        if (g_markup_parse_context_parse(context, text, length, NULL) == FALSE || _list->getSize() == 0)
            return NULL;
    }

    printf("Number of launcher sections = %d\n", _list->getSize());

    // make sure the section is ended
    endSection();

    g_free(text);
    g_markup_parse_context_free (context);

    // Use the last section to be the catch all section if was't defined
    if ( _extraSection == NULL ) 
        _originExtraSection = _extraSection = _list->get( _list->getSize() - 1 );

    _currentSection = _extraSection;

    // add desktop files that are not the applications.menu
    if ( g_hash_table_size ( _table ) > 0 )
        g_hash_table_foreach ( _table, addExtraApplication, this );

    endSection();

    g_hash_table_destroy(_table);


    assignSectionIcons();


    return _list;
}

bool MenuReader::hasChange()
{
    if (_fd < 0 ) return FALSE;

    struct timeval time;
    fd_set rfds;
    int ret;

    time.tv_sec = 0;
    time.tv_usec = 100;

    /* zero-out the fd_set */
    FD_ZERO (&rfds);

    FD_SET (_fd, &rfds);

    ret = select (_fd + 1, &rfds, NULL, NULL, &time);
    if (ret < 0)
        return FALSE;
    else if (!ret)
        return FALSE;
    else if (FD_ISSET (_fd, &rfds) )
    {
        /* inotify events are available! */
        int length, i = 0;
        char buffer[BUF_LEN];

        length = read( _fd, buffer, BUF_LEN );
        if ( length < 0 )
            return FALSE;

        while ( i < length )
        {
            struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];

            if ( event->wd == _applicationMenuWD && event->mask & IN_MODIFY )
            {
                return TRUE;
            }
            else if ( event->wd == _desktopsFileWD )
        {
            return TRUE;
        }

            i += EVENT_SIZE + event->len;
        }
    }
    return FALSE;
}



void MenuReader::assignSectionIcons()
{
    GKeyFile *kfile = g_key_file_new();
    g_key_file_load_from_file(kfile, "/home/user/.telescope.cats", G_KEY_FILE_NONE, 0);

    for (unsigned int i = 0; i < _list->getSize(); i++)
    {
        char *iconfile = g_key_file_get_string(kfile, "Icons", _list->get(i)->getName(), 0);
        if (iconfile)
        {
            Image *icon = new Image(_dpy, iconfile);
            _list->get(i)->setIcon(icon, true);
        }

        free(iconfile);


        _list->get(i)->setIconChangedCallback(saveSectionIcon, this);
    }

    g_key_file_free(kfile);
}


void MenuReader::saveSectionIcon(Section *section, void *data)
{
    GKeyFile *kfile = g_key_file_new();
    g_key_file_load_from_file(kfile, "/home/user/.telescope.cats", G_KEY_FILE_NONE, 0);

    if (section->getIcon())
        g_key_file_set_string(kfile,
            "Icons",
            section->getName(),
            section->getIcon()->filename()
        );
    else
        g_key_file_remove_key(kfile, "Icons", section->getName(), 0);

    FILE *f = fopen("/home/user/.telescope.cats", "w");
    if (f == 0)
        fprintf(stderr, "Could not open /home/user/.telescope.cats for writing!\n");
    else
    {
        gsize length;
        char *content = g_key_file_to_data(kfile, &length, 0);

        fwrite(content, length, 1, f);

        fclose(f);

        free(content);
    }

    g_key_file_free(kfile);
}
