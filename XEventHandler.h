#ifndef __TELESCOPE_XEVENTHANDLER_H
#define __TELESCOPE_XEVENTHANDLER_H


class XEventHandler
{
    public:
        virtual void onEvent(XEvent *event) = 0;
};


#endif
