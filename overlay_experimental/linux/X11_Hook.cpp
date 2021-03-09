#include "X11_Hook.h"
#include "../Renderer_Detector.h"
#include "../dll/dll.h"

#ifdef __LINUX__
#ifdef EMU_OVERLAY

#include <imgui.h>
#include <impls/linux/imgui_impl_x11.h>

#include <dlfcn.h>
#include <unistd.h>
#include <fstream>

extern int ImGui_ImplX11_EventHandler(XEvent &event);

X11_Hook* X11_Hook::_inst = nullptr;

bool X11_Hook::start_hook()
{
    bool res = true;
    if (!hooked)
    {
        PRINT_DEBUG("Hooked X11\n");
        hooked = true;
    }
    return res;
}

void X11_Hook::resetRenderState()
{
    if (initialized)
    {
        game_wnd = 0;
        initialized = false;
        ImGui_ImplX11_Shutdown();
    }
}

void X11_Hook::prepareForOverlay(Display *display, Window wnd)
{
    if (!initialized)
    {
        ImGui_ImplX11_Init(display, (void*)wnd);

        game_wnd = wnd;

        initialized = true;
    }

    ImGui_ImplX11_NewFrame();
}

/////////////////////////////////////////////////////////////////////////////////////
// X11 window hooks
bool IgnoreEvent(XEvent &event)
{
    switch(event.type)
    {
        // Keyboard
        case KeyPress: case KeyRelease:
        // MouseButton
        case ButtonPress: case ButtonRelease:
        // Mouse move
        case MotionNotify:
            return true;
    }
    return false;
}

int X11_Hook::check_for_overlay(Display *d, int num_events)
{
    static Time prev_time = {};

    X11_Hook* inst = Inst();

    if( inst->initialized )
    {
        XEvent event;
        while(num_events)
        {
            //inst->_XPeekEvent(d, &event);
            XPeekEvent(d, &event);

            Steam_Overlay* overlay = get_steam_client()->steam_overlay;
            bool show = overlay->ShowOverlay();
            // Is the event is a key press
            if (event.type == KeyPress)
            {
                // Tab is pressed and was not pressed before
                //if (event.xkey.keycode == inst->_XKeysymToKeycode(d, XK_Tab) && event.xkey.state & ShiftMask)
                if (event.xkey.keycode == XKeysymToKeycode(d, XK_Tab) && event.xkey.state & ShiftMask)
                {
                    // if key TAB is held, don't make the overlay flicker :p
                    if( event.xkey.time != prev_time)
                    {
                        overlay->ShowOverlay(!overlay->ShowOverlay());

                        if (overlay->ShowOverlay())
                            show = true;
                    }
                }
            }
            //else if(event.type == KeyRelease && event.xkey.keycode == inst->_XKeysymToKeycode(d, XK_Tab))
            else if(event.type == KeyRelease && event.xkey.keycode == XKeysymToKeycode(d, XK_Tab))
            {
                prev_time = event.xkey.time;
            }

            if (show)
            {
                ImGui_ImplX11_EventHandler(event);

                if (IgnoreEvent(event))
                {
                    //inst->_XNextEvent(d, &event);
                    XNextEvent(d, &event);
                    --num_events;
                }
                else
                    break;
            }
            else
                break;
        }
    }
    return num_events;
}

int X11_Hook::MyXEventsQueued(Display *display, int mode)
{
    X11_Hook* inst = X11_Hook::Inst();

    int res = inst->_XEventsQueued(display, mode);

    if( res )
    {
        res = inst->check_for_overlay(display, res);
    }

    return res;
}

int X11_Hook::MyXNextEvent(Display* display, XEvent *event)
{
    return Inst()->_XNextEvent(display, event);
}

int X11_Hook::MyXPeekEvent(Display* display, XEvent *event)
{
    return Inst()->_XPeekEvent(display, event);
}

int X11_Hook::MyXPending(Display* display)
{
    int res = Inst()->_XPending(display);

    if( res )
    {
        res = Inst()->check_for_overlay(display, res);
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////////////////

X11_Hook::X11_Hook() :
    initialized(false),
    hooked(false),
    game_wnd(0),
    _XEventsQueued(nullptr),
    _XPeekEvent(nullptr),
    _XNextEvent(nullptr),
    _XPending(nullptr)
{
    //_library = dlopen(DLL_NAME, RTLD_NOW);
}

X11_Hook::~X11_Hook()
{
    PRINT_DEBUG("X11 Hook removed\n");

    resetRenderState();

    //dlclose(_library);

    _inst = nullptr;
}

X11_Hook* X11_Hook::Inst()
{
    if (_inst == nullptr)
        _inst = new X11_Hook;

    return _inst;
}

const char* X11_Hook::get_lib_name() const
{
    return DLL_NAME;
}

#endif//EMU_OVERLAY
#endif//#__LINUX__
