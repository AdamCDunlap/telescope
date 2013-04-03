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

#include "SectionList.h"

SectionList::SectionList()
{
    _sections = g_ptr_array_new();
}

SectionList::~SectionList()
{
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "SectionList dtor");
    for (guint i = 0; i < _sections->len; i++)
        delete get(i);

    g_ptr_array_free(_sections, TRUE);
}

guint SectionList::getSize()
{
    return _sections->len;
}

Section * SectionList::get(guint index)
{
    return (Section *) g_ptr_array_index(_sections, index);
}

void SectionList::add(Section *section)
{
    g_ptr_array_add(_sections, (gpointer) section);
}

bool SectionList::has(Section *section)
{
    for ( uint i = 0; i < _sections->len; i++ )
    {
        if ( (Section * ) g_ptr_array_index ( _sections, i ) == section )
            return true;
    }
    return false;
}
