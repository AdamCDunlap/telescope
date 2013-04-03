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

#ifndef MENUREADER_H
#define MENUREADER_H

#include "SectionList.h"

class MenuReader
{
public:
    MenuReader(Display *dpy);
    ~MenuReader();
    static MenuReader * getInstance() { return _instance; }
    SectionList * processMenu();
    void beginSection(const gchar *name);
    void addApplication(const gchar *desktopFilename, bool isExtra = false);
    void endSection();
    void setCurrentSectionAsCatchAll();
    bool hasChange();

    SectionList *sectionList() { return _list; }

protected:
    void _init();
    void _cleanup();
    void _getDesktopFiles();

private:
    static MenuReader * _instance;
    Display *_dpy;
    Section *_currentSection;
    Section *_extraSection;
    Section *_originExtraSection;
    SectionList *_list;
    guint _partNo;
    int _fd;
    int _applicationMenuWD;
    int _desktopsFileWD;
    GHashTable * _table;

    void assignSectionIcons();
    static void saveSectionIcon(Section *section, void *data);
};

#endif // MENUREADER_H
