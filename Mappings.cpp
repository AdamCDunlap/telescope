//
// Telescope - graphical task switcher
//
// (c) Ilya Skriblovsky, 2010
// <Ilya.Skriblovsky@gmail.com>
//

// $Id: Mappings.cpp 158 2011-02-22 20:05:07Z mitrandir $

#include "Mappings.h"

#include <string.h>
#include <stdlib.h>

#include "Mapping.h"


const char *mappingsFile = "/etc/telescope.keys";


Mappings::Mappings(Display *dpy)
    :_dpy(dpy)
{
    loadMappings();
}

void Mappings::loadMappings()
{
    FILE *f = fopen(mappingsFile, "r");

    if (! f)
        fprintf(stderr, "Mappings file not found: %s\n", mappingsFile);
    else
    {
        int lineNo = 1;
        while (! feof(f))
        {
            char line[256];
            const char *error;

            if (fgets(line, sizeof(line), f) != 0)
            {
                Mapping *mapping = Mapping::parseMappingLine(_dpy, line, &error);
                if (mapping)
                    _mappings.append(mapping);
                else
                    if (error != 0)
                        fprintf(stderr, "Error parsing mapping file (line %d): %s\n", lineNo, error);
            }

            lineNo++;
        }

        fclose(f);
    }
}


Mappings::~Mappings()
{
    for (LinkedList<Mapping*>::Iter i = _mappings.head(); i; ++i)
        delete (*i);
}



void Mappings::handleEvent(TeleWindow *teleWindow, Mapping::Event event, KeyCode keyCode)
{
    for (LinkedList<Mapping*>::Iter i = _mappings.head(); i; ++i)
        (*i)->handleEvent(teleWindow, event, keyCode);
}
