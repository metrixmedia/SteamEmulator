#ifndef __INCLUDED_X11_HOOK_H__
#define __INCLUDED_X11_HOOK_H__

#include "../Base_Hook.h"

#ifdef __LINUX__
#ifdef EMU_OVERLAY

#include <X11/X.h> // XEvent types
#include <X11/Xlib.h> // XEvent structure

extern "C" int XEventsQueued(Display *display, int mode);
extern "C" int XPending(Display *display);

class X11_Hook : public Base_Hook
{
public:
    friend int XEventsQueued(Display *display, int mode);
    friend int XPending(Display *display);
    static constexpr const char* DLL_NAME = "libX11.so";

private:
    static X11_Hook* _inst;

    // Variables
    bool hooked;
    bool initialized;
    Window game_wnd;

    // Functions
    X11_Hook();
    int check_for_overlay(Display *d, int num_events);

    // Hook to X11 window messages
    decltype(XEventsQueued)* _XEventsQueued;
    decltype(XPeekEvent)* _XPeekEvent;
    decltype(XNextEvent)* _XNextEvent;
    decltype(XPending)* _XPending;
    //decltype(XKeysymToKeycode)* _XKeysymToKeycode;
    //decltype(XLookupKeysym)* _XLookupKeysym;
    //decltype(XGetGeometry)* _XGetGeometry;

    static int MyXEventsQueued(Display * display, int mode);
    static int MyXNextEvent(Display* display, XEvent *event);
    static int MyXPeekEvent(Display* display, XEvent *event);
    static int MyXPending(Display* display);

public:
    virtual ~X11_Hook();

    void resetRenderState();
    void prepareForOverlay(Display *display, Window wnd);

    Window get_game_wnd() const{ return game_wnd; }

    bool start_hook();
    static X11_Hook* Inst();
    virtual const char* get_lib_name() const;
};

#endif//EMU_OVERLAY
#endif//__LINUX__
#endif//__INCLUDED_X11_HOOK_H__
