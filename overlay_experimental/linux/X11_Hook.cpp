/*
 * Copyright (C) 2019-2020 Nemirtingas
 * This file is part of the ingame overlay project
 *
 * The ingame overlay project is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * The ingame overlay project is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the ingame overlay project; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "X11_Hook.h"

#include <imgui.h>
#include <backends/imgui_impl_x11.h>
#include <System/Library.h>

extern int ImGui_ImplX11_EventHandler(XEvent &event);

constexpr decltype(X11_Hook::DLL_NAME) X11_Hook::DLL_NAME;

X11_Hook* X11_Hook::_inst = nullptr;

uint32_t ToggleKeyToNativeKey(ingame_overlay::ToggleKey k)
{
    struct {
        ingame_overlay::ToggleKey lib_key;
        uint32_t native_key;
    } mapping[] = {
        { ingame_overlay::ToggleKey::ALT  , XK_Alt_L     },
        { ingame_overlay::ToggleKey::CTRL , XK_Control_L },
        { ingame_overlay::ToggleKey::SHIFT, XK_Shift_L   },
        { ingame_overlay::ToggleKey::TAB  , XK_Tab       },
        { ingame_overlay::ToggleKey::F1   , XK_F1        },
        { ingame_overlay::ToggleKey::F2   , XK_F2        },
        { ingame_overlay::ToggleKey::F3   , XK_F3        },
        { ingame_overlay::ToggleKey::F4   , XK_F4        },
        { ingame_overlay::ToggleKey::F5   , XK_F5        },
        { ingame_overlay::ToggleKey::F6   , XK_F6        },
        { ingame_overlay::ToggleKey::F7   , XK_F7        },
        { ingame_overlay::ToggleKey::F8   , XK_F8        },
        { ingame_overlay::ToggleKey::F9   , XK_F9        },
        { ingame_overlay::ToggleKey::F10  , XK_F10       },
        { ingame_overlay::ToggleKey::F11  , XK_F11       },
        { ingame_overlay::ToggleKey::F12  , XK_F12       },
    };

    for (auto const& item : mapping)
    {
        if (item.lib_key == k)
            return item.native_key;
    }

    return 0;
}

bool GetKeyState(Display* d, KeySym keySym, char szKey[32])
{
    int iKeyCodeToFind = XKeysymToKeycode(d, keySym);

    return szKey[iKeyCodeToFind / 8] & (1 << (iKeyCodeToFind % 8));
}

bool X11_Hook::StartHook(std::function<bool(bool)>& _key_combination_callback, std::set<ingame_overlay::ToggleKey> const& toggle_keys)
{
    if (!_Hooked)
    {
        if (!_key_combination_callback)
        {
            SPDLOG_ERROR("Failed to hook X11: No key combination callback.");
            return false;
        }

        if (toggle_keys.empty())
        {
            SPDLOG_ERROR("Failed to hook X11: No key combination.");
            return false;
        }

        void* hX11 = System::Library::GetLibraryHandle(DLL_NAME);
        if (hX11 == nullptr)
        {
            SPDLOG_WARN("Failed to hook X11: Cannot find {}", DLL_NAME);
            return false;
        }

        System::Library::Library libX11;
        LibraryName = System::Library::GetLibraryPath(hX11);

        if (!libX11.OpenLibrary(LibraryName, false))
        {
            SPDLOG_WARN("Failed to hook X11: Cannot load {}", LibraryName);
            return false;
        }

        struct {
            void** func_ptr;
            void* hook_ptr;
            const char* func_name;
        } hook_array[] = {
            { (void**)&XEventsQueued, &X11_Hook::MyXEventsQueued, "XEventsQueued" },
            { (void**)&XPending     , &X11_Hook::MyXPending     , "XPending"      },
        };

        for (auto& entry : hook_array)
        {
            *entry.func_ptr = libX11.GetSymbol<void*>(entry.func_name);
            if (entry.func_ptr == nullptr)
            {
                SPDLOG_ERROR("Failed to hook X11: Event function {} missing.", entry.func_name);
                return false;
            }
        }

        SPDLOG_INFO("Hooked X11");

        _KeyCombinationCallback = std::move(_key_combination_callback);
        
        for (auto& key : toggle_keys)
        {
            uint32_t k = ToggleKeyToNativeKey(key);
            if (k != 0)
            {
                _NativeKeyCombination.insert(k);
            }
        }

        _Hooked = true;

        BeginHook();
        
        for (auto& entry : hook_array)
        {
            HookFunc(std::make_pair(entry.func_ptr, entry.hook_ptr));
        }

        EndHook();
    }
    return true;
}

void X11_Hook::ResetRenderState()
{
    if (_Initialized)
    {
        _GameWnd = 0;
        _Initialized = false;
        ImGui_ImplX11_Shutdown();
    }
}

void X11_Hook::SetInitialWindowSize(Display* display, Window wnd)
{
    unsigned int width, height;
    Window unused_window;
    int unused_int;
    unsigned int unused_unsigned_int;

    XGetGeometry(display, wnd, &unused_window, &unused_int, &unused_int, &width, &height, &unused_unsigned_int, &unused_unsigned_int);

    ImGui::GetIO().DisplaySize = ImVec2((float)width, (float)height);
}

bool X11_Hook::PrepareForOverlay(Display *display, Window wnd)
{
    if(!_Hooked)
        return false;

    if (_GameWnd != wnd)
        ResetRenderState();

    if (!_Initialized)
    {
        ImGui_ImplX11_Init(display, (void*)wnd);
        _GameWnd = wnd;

        _Initialized = true;
    }

    ImGui_ImplX11_NewFrame();

    return true;
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
        // Copy to clipboard request
        case SelectionRequest:
            return true;
    }
    return false;
}

int X11_Hook::_CheckForOverlay(Display *d, int num_events)
{
    static Time prev_time = {};
    X11_Hook* inst = Inst();

    char szKey[32];

    if( _Initialized )
    {
        XEvent event;
        while(num_events)
        {
            bool skip_input = _KeyCombinationCallback(false);

            XPeekEvent(d, &event);
            ImGui_ImplX11_EventHandler(event);

            // Is the event is a key press
            if (event.type == KeyPress || event.type == KeyRelease)
            {
                XQueryKeymap(d, szKey);
                int key_count = 0;
                for (auto const& key : inst->_NativeKeyCombination)
                {
                    if (GetKeyState(d, key, szKey))
                        ++key_count;
                }

                if (key_count == inst->_NativeKeyCombination.size())
                {// All shortcut keys are pressed
                    if (!inst->_KeyCombinationPushed)
                    {
                        if (inst->_KeyCombinationCallback(true))
                        {
                            skip_input = true;
                            // Save the last known cursor pos when opening the overlay
                            // so we can spoof the GetCursorPos return value.
                            //inst->GetCursorPos(&inst->_SavedCursorPos);
                        }
                        inst->_KeyCombinationPushed = true;
                    }
                }
                else
                {
                    inst->_KeyCombinationPushed = false;
                }
            }

            if (!skip_input || !IgnoreEvent(event))
            {
                if(num_events)
                    num_events = 1;
                break;
            }

            XNextEvent(d, &event);
            --num_events;
        }
    }
    return num_events;
}

int X11_Hook::MyXEventsQueued(Display *display, int mode)
{
    X11_Hook* inst = X11_Hook::Inst();

    int res = inst->XEventsQueued(display, mode);

    if( res )
    {
        res = inst->_CheckForOverlay(display, res);
    }

    return res;
}

int X11_Hook::MyXPending(Display* display)
{
    int res = Inst()->XPending(display);

    if( res )
    {
        res = Inst()->_CheckForOverlay(display, res);
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////////////////

X11_Hook::X11_Hook() :
    _Initialized(false),
    _Hooked(false),
    _GameWnd(0),
    _KeyCombinationPushed(false),
    XEventsQueued(nullptr),
    XPending(nullptr)
{
}

X11_Hook::~X11_Hook()
{
    SPDLOG_INFO("X11 Hook removed");

    ResetRenderState();

    _inst = nullptr;
}

X11_Hook* X11_Hook::Inst()
{
    if (_inst == nullptr)
        _inst = new X11_Hook;

    return _inst;
}

std::string X11_Hook::GetLibraryName() const
{
    return LibraryName;
}

