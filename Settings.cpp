//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: Settings.cpp 175 2011-03-01 11:32:59Z mitrandir $

#include "Settings.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define CONFIG_FILE     "/etc/telescope.conf"


Settings* Settings::_instance = 0;

Settings* Settings::instance() { return _instance; }


Settings::Settings()
{
    _instance = this;

    // Default values
    _scrollingEnabled = false;

    _backgroundFilename = 0; // Will be set later if not in config file
    _backgroundMode = Stretched;
    _backgroundColor = strdup("#202020");

    _headerLeftFilename = strdup("/usr/share/telescope/header-left.png");
    _headerRightFilename = strdup("/usr/share/telescope/header-right.png");
    _headerMiddleFilename = strdup("/usr/share/telescope/header-middle.png");
    _headerLeftSelectedFilename = strdup("/usr/share/telescope/header-left-selected.png");
    _headerRightSelectedFilename = strdup("/usr/share/telescope/header-right-selected.png");
    _headerMiddleSelectedFilename = strdup("/usr/share/telescope/header-middle-selected.png");
    _borderColor = strdup("#215173");
    _borderActiveColor = strdup("#315994");

    _brokenPatternFilename = strdup("/usr/share/telescope/broken-pattern.png");

    _textBackgroundFilename = strdup("/usr/share/telescope/text-background.png");

    _panelBackgroundFilename = strdup("/usr/share/telescope/panel.png");
    _panelFocusLeftFilename = strdup("/usr/share/telescope/panel-focus-left.png");
    _panelFocusRightFilename = strdup("/usr/share/telescope/panel-focus-right.png");
    _panelFocusMiddleFilename = strdup("/usr/share/telescope/panel-focus-middle.png");

    _borderWidth = 3;
    _textLeftMargin = 7;
    _textRightMargin = 27;
    _closeButtonXSpan = 40;
    _closeButtonYSpan = 40;

    _fontSize = 24;
    _textYOffset = -7;

    _disableSelection = false;

    _showDesktopThumbnail = false;

    _showDesktopByIconify = false;

    _hotKey = strdup("F5");


    #ifdef LAUNCHER
        _disableLauncher = false;
    #endif

    _categoryIconsDir = strdup("/usr/share/telescope/category-icons/");


    // Reading
    FILE *f = fopen(CONFIG_FILE, "r");
    if (f == 0)
        fprintf(stderr, "Config file not found: " CONFIG_FILE "\n");
    else
    {
        while (! feof(f))
        {
            char line[256];
            if (fgets(line, sizeof(line), f) != 0)
                parseLine(line);
        }


        fclose(f);
    }


    // Use hildon background if no background specified in config file
    if (_backgroundFilename == 0)
    {
        FILE *homeBgFile = fopen("/home/user/.osso/hildon-desktop/home-background.conf", "r");
        if (homeBgFile)
        {
            char line[1024];
            static const char *prefix1 = "BackgroundImage=";
            static const char *prefix3 = "CachedAs=";
            int r = 0, g = 0, b = 0;
            while (fgets(line, sizeof(line), homeBgFile) != 0)
            {
                if (strstr(line, prefix1) == line)
                {
                    char tmp[1024];
                    int len = strcspn( &line[strlen(prefix1)], "\n");
                    strncpy(
                        tmp,
                        &line[strlen(prefix1)],
                        len
                    );
                    tmp[len] = '\0';
                    _backgroundFilename = urldecode(tmp);
                }
                else if (strstr(line, prefix3) == line)
                {
                    int valuelen = strcspn(&line[strlen(prefix3)], "\n");
                    if (valuelen > 0)
                    {
                        free(_backgroundFilename);
                        _backgroundFilename = strndup(&line[strlen(prefix3)], valuelen);
                    }
                }
                else if (strstr(line, "Mode=") == line)
                {
                    if (strcmp(&line[5], "Stretched\n") == 0)
                        _backgroundMode = Stretched;
                    else if (strcmp(&line[5], "Centered\n") == 0)
                        _backgroundMode = Centered;
                    else if (strcmp(&line[5], "Scaled\n") == 0)
                        _backgroundMode = Scaled;
                    else if (strcmp(&line[5], "Cropped\n") == 0)
                        _backgroundMode = Cropped;
                }
                else if (strncmp(line, "Red=", 4) == 0)
                    r = atoi(&line[4]) / 256;
                else if (strncmp(line, "Green=", 6) == 0)
                    g = atoi(&line[6]) / 256;
                else if (strncmp(line, "Blue=", 5) == 0)
                    b = atoi(&line[5]) / 256;
            }

            free(_backgroundColor);
            _backgroundColor = (char*)malloc(8);
            snprintf(_backgroundColor, 8, "#%02x%02x%02x", r, g, b);

            fclose(homeBgFile);
        }

        if (_backgroundFilename == 0)
            _backgroundFilename = (char*)calloc(1, 1); // Emtpy string fallback
    }
}

Settings::~Settings()
{
    free(_backgroundFilename);
    free(_backgroundColor);
    free(_headerLeftFilename);
    free(_headerRightFilename);
    free(_headerMiddleFilename);
    free(_headerLeftSelectedFilename);
    free(_headerRightSelectedFilename);
    free(_headerMiddleSelectedFilename);
    free(_borderColor);
    free(_borderActiveColor);
    free(_textBackgroundFilename);

    free(_panelBackgroundFilename);
    free(_panelFocusLeftFilename);
    free(_panelFocusRightFilename);
    free(_panelFocusMiddleFilename);

    free(_categoryIconsDir);
}



int hexdigit(char hex)
{
    if (hex >= '0' && hex <= '9')
        return hex - '0';

    if (hex >= 'A' && hex <= 'F')
        return hex - 'A' + 0xA;

    if (hex >= 'a' && hex <= 'f')
        return hex - 'a' + 0xA;

    return 0;
}



char* Settings::urldecode(const char *url)
{
    if (strstr(url, "file://") == url)
        url += 7;

    char *result = (char*)malloc(strlen(url) + 1);
    char *r = result;

    while (*url)
    {
        if (*url == '%')
        {
            ++url;
            int val = hexdigit(*url++);
            val <<= 4;
            val |= hexdigit(*url++);
            *r++ = val;
        }
        else
            *r++ = *url++;
    }

    *r = '\0';


    return result;
}



#define SPACECHARS  " \t"

void Settings::parseLine(const char *line)
{
    const char *key = &line[strspn(line, SPACECHARS)];

    if (*key == '\n')
        return; // Empty line

    if (*key == '\0')
        return; // Empty line

    if (*key == '#')
        return; // Comment

    int keylen = strcspn(key, " =:");


    const char *value = &key[keylen + strspn(&key[keylen], " =:")];
    int valuelen = strcspn(value, "\n");

    if (keylen >= 200) keylen = 199;
    if (valuelen >= 200) valuelen = 199;

    char keybuf[200];
    strncpy(keybuf, key, keylen);
    keybuf[keylen] = '\0';
    char valuebuf[200];
    strncpy(valuebuf, value, valuelen);
    valuebuf[valuelen] = '\0';


    parseOpt(keybuf, valuebuf);
}


void Settings::parseOpt(const char *key, const char *value)
{
    printf("[Opt]\t'%s' = '%s'\n", key, value);

    if (strcmp(key, "scrolling.enabled") == 0)
    {
        _scrollingEnabled = parseBool(value);

        if (_scrollingEnabled)
            printf("Scrolling enabled\n");
        else
            printf("Scrolling disabled\n");
    }
    else if (strcmp(key, "background.filename") == 0)
    {
        free(_backgroundFilename);
        _backgroundFilename = strdup(value);

        printf("Background: '%s'\n", _backgroundFilename);
    }
    else if (strcmp(key, "background.mode") == 0)
    {
        if (strcmp(value, "stretched") == 0)
            _backgroundMode = Stretched;
        else if (strcmp(value, "centered") == 0)
            _backgroundMode = Centered;
        else if (strcmp(value, "scaled") == 0)
            _backgroundMode = Scaled;
        else if (strcmp(value, "cropped") == 0)
            _backgroundMode = Cropped;
    }
    else if (strcmp(key, "background.color") == 0)
    {
        free(_backgroundColor);
        _backgroundColor = strdup(value);

        printf("Background color: %s\n", _backgroundColor);
    }
    else if (strcmp(key, "header.left.filename") == 0)
    {
        free(_headerLeftFilename);
        _headerLeftFilename = strdup(value);
    }
    else if (strcmp(key, "header.right.filename") == 0)
    {
        free(_headerRightFilename);
        _headerRightFilename = strdup(value);
    }
    else if (strcmp(key, "header.middle.filename") == 0)
    {
        free(_headerMiddleFilename);
        _headerMiddleFilename = strdup(value);
    }
    else if (strcmp(key, "header.left.selected.filename") == 0)
    {
        free(_headerLeftSelectedFilename);
        _headerLeftFilename = strdup(value);
    }
    else if (strcmp(key, "header.right.selected.filename") == 0)
    {
        free(_headerRightSelectedFilename);
        _headerRightFilename = strdup(value);
    }
    else if (strcmp(key, "header.middle.selected.filename") == 0)
    {
        free(_headerMiddleSelectedFilename);
        _headerMiddleFilename = strdup(value);
    }
    else if (strcmp(key, "border.color") == 0)
    {
        free(_borderColor);
        _borderColor = strdup(value);
    }
    else if (strcmp(key, "border.color.active") == 0)
    {
        free(_borderActiveColor);
        _borderActiveColor = strdup(value);
    }
    else if (strcmp(key, "broken.pattern") == 0)
    {
        free(_brokenPatternFilename);
        _brokenPatternFilename = strdup(value);
    }
    #ifdef LAUNCHER
        else if (strcmp(key, "launcher.text.background") == 0)
        {
            free(_textBackgroundFilename);
            _textBackgroundFilename = strdup(value);
        }
        else if (strcmp(key, "launcher.panel.background.filename") == 0)
        {
            free(_panelBackgroundFilename);
            _panelBackgroundFilename = strdup(value);
        }
        else if (strcmp(key, "launcher.panel.focus.left.filename") == 0)
        {
            free(_panelFocusLeftFilename);
            _panelFocusLeftFilename = strdup(value);
        }
        else if (strcmp(key, "launcher.panel.focus.right.filename") == 0)
        {
            free(_panelFocusRightFilename);
            _panelFocusRightFilename = strdup(value);
        }
        else if (strcmp(key, "launcher.panel.focus.middle.filename") == 0)
        {
            free(_panelFocusMiddleFilename);
            _panelFocusMiddleFilename = strdup(value);
        }
        else if (strcmp(key, "launcher.disable") == 0)
            _disableLauncher = parseBool(value);
        else if (strcmp(key, "launcher.categories.iconsdir") == 0)
        {
            free(_categoryIconsDir);
            _categoryIconsDir = strdup(value);
        }
    #endif
    else if (strcmp(key, "border.width") == 0)
        _borderWidth = atoi(value);
    else if (strcmp(key, "text.leftMargin") == 0)
        _textLeftMargin = atoi(value);
    else if (strcmp(key, "text.rightMargin") == 0)
        _textRightMargin = atoi(value);
    else if (strcmp(key, "closeButton.xSpan") == 0)
        _closeButtonXSpan = atoi(value);
    else if (strcmp(key, "closeButton.ySpan") == 0)
        _closeButtonYSpan = atoi(value);
    else if (strcmp(key, "text.font.size") == 0)
        _fontSize = atoi(value);
    else if (strcmp(key, "text.yOffset") == 0)
        _textYOffset = atoi(value);
    else if (strcmp(key, "selection.disabled") == 0)
        _disableSelection = parseBool(value);
    else if (strcmp(key, "show.desktop.thumbnail") == 0)
        _showDesktopThumbnail = parseBool(value);
    else if (strcmp(key, "show.desktop.iconify") == 0)
        _showDesktopByIconify = parseBool(value);
    else if (strcmp(key, "hotkey") == 0)
    {
        free(_hotKey);
        _hotKey = strdup(value);
    }
}


bool Settings::parseBool(const char *value)
{
    if (*value == '1')
        return true;
    if (strcmp(value, "on") == 0)
        return true;
    if (strcmp(value, "On") == 0)
        return true;
    if (strcmp(value, "yes") == 0)
        return true;
    if (strcmp(value, "Yes") == 0)
        return true;

    return false;
}
