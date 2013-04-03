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

#ifndef SECTIONLIST_H
#define SECTIONLIST_H

#include "Section.h"

class SectionList
{
public:
    SectionList();
    ~SectionList();
    guint getSize();
    Section * get(guint index);
    void add(Section *section);
    bool has(Section *section);

private:
    GPtrArray* _sections;
};

#endif // SECTIONLIST_H

