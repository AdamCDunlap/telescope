//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: Settings.h 175 2011-03-01 11:32:59Z mitrandir $

// Settings - loads and provides settings from configuration file

#ifndef __TELESCOPE__SETTINGS_H
#define __TELESCOPE__SETTINGS_H

class Settings
{
    public:
        enum BackgroundMode
        {
            Stretched,
            Centered,
            Scaled,
            Cropped
        };

    private:
        static Settings *_instance;

        bool _scrollingEnabled;
        char *_backgroundFilename;
        BackgroundMode _backgroundMode;
        char *_backgroundColor;

        char *_headerLeftFilename;
        char *_headerRightFilename;
        char *_headerMiddleFilename;
        char *_headerLeftSelectedFilename;
        char *_headerRightSelectedFilename;
        char *_headerMiddleSelectedFilename;
        char *_borderColor;
        char *_borderActiveColor;

        char *_brokenPatternFilename;

        char *_textBackgroundFilename;

        char *_panelBackgroundFilename;
        char *_panelFocusLeftFilename;
        char *_panelFocusRightFilename;
        char *_panelFocusMiddleFilename;

        int _borderWidth;
        int _textLeftMargin;
        int _textRightMargin;
        int _closeButtonXSpan;
        int _closeButtonYSpan;

        int _fontSize;
        int _textYOffset;

        char *_hotKey;


        bool _disableSelection;

        bool _showDesktopThumbnail;

        bool _showDesktopByIconify;


        #ifdef LAUNCHER
            bool _disableLauncher;
        #endif


        char *_categoryIconsDir;
        char *_categorySettingsFile;


        void parseLine(const char *line);
        void parseOpt(const char *key, const char *value);

        bool parseBool(const char *value);

        char* urldecode(const char *url);

    public:
        Settings();
        ~Settings();

        static Settings* instance();

        bool scrollingEnabled() { return _scrollingEnabled; }

        const char* backgroundFilename() { return _backgroundFilename; }
        BackgroundMode backgroundMode() { return _backgroundMode; }
        char* backgroundColor() { return _backgroundColor; }

        const char* headerLeftFilename() { return _headerLeftFilename; }
        const char* headerRightFilename() { return _headerRightFilename; }
        const char* headerMiddleFilename() { return _headerMiddleFilename; }
        const char* headerLeftSelectedFilename() { return _headerLeftSelectedFilename; }
        const char* headerRightSelectedFilename() { return _headerRightSelectedFilename; }
        const char* headerMiddleSelectedFilename() { return _headerMiddleSelectedFilename; }
        char* borderColor() { return _borderColor; }
        char* borderActiveColor() { return _borderActiveColor; }

        const char *brokenPatternFilename() { return _brokenPatternFilename; }

        const char *textBackgroundFilename() { return _textBackgroundFilename; }

        const char *panelBackgroundFilename() { return _panelBackgroundFilename; }
        const char *panelFocusLeftFilename() { return _panelFocusLeftFilename; }
        const char *panelFocusRightFilename() { return _panelFocusRightFilename; }
        const char *panelFocusMiddleFilename() { return _panelFocusMiddleFilename; }

        int borderWidth() { return _borderWidth; }
        int textLeftMargin() { return _textLeftMargin; }
        int textRightMargin() { return _textRightMargin; }
        int closeButtonXSpan() { return _closeButtonXSpan; }
        int closeButtonYSpan() { return _closeButtonYSpan; }

        int fontSize() { return _fontSize; }
        int textYOffset() { return _textYOffset; }

        bool disableSelection() { return _disableSelection; }

        bool showDesktopThumbnail() { return _showDesktopThumbnail; }

        bool showDesktopByIconify() { return _showDesktopByIconify; }


        const char *hotKey() { return _hotKey; }


        bool disableLauncher()
        {
            #ifdef LAUNCHER
                return _disableLauncher;
            #else
                return true;
            #endif
        }


        const char *categoryIconsDir() { return _categoryIconsDir; }
};

#endif
