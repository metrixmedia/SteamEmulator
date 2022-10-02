/*
 * Copyright (C) Nemirtingas
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

#include "Windows_Hook.h"

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <System/Library.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

constexpr decltype(Windows_Hook::DLL_NAME) Windows_Hook::DLL_NAME;

Windows_Hook* Windows_Hook::_inst = nullptr;

int ToggleKeyToNativeKey(ingame_overlay::ToggleKey k)
{
    struct {
        ingame_overlay::ToggleKey lib_key;
        int native_key;
    } mapping[] = {
        { ingame_overlay::ToggleKey::ALT  , VK_MENU    },
        { ingame_overlay::ToggleKey::CTRL , VK_CONTROL },
        { ingame_overlay::ToggleKey::SHIFT, VK_SHIFT   },
        { ingame_overlay::ToggleKey::TAB  , VK_TAB     },
        { ingame_overlay::ToggleKey::F1   , VK_F1      },
        { ingame_overlay::ToggleKey::F2   , VK_F2      },
        { ingame_overlay::ToggleKey::F3   , VK_F3      },
        { ingame_overlay::ToggleKey::F4   , VK_F4      },
        { ingame_overlay::ToggleKey::F5   , VK_F5      },
        { ingame_overlay::ToggleKey::F6   , VK_F6      },
        { ingame_overlay::ToggleKey::F7   , VK_F7      },
        { ingame_overlay::ToggleKey::F8   , VK_F8      },
        { ingame_overlay::ToggleKey::F9   , VK_F9      },
        { ingame_overlay::ToggleKey::F10  , VK_F10     },
        { ingame_overlay::ToggleKey::F11  , VK_F11     },
        { ingame_overlay::ToggleKey::F12  , VK_F12     },
    };

    for (auto const& item : mapping)
    {
        if (item.lib_key == k)
            return item.native_key;
    }

    return 0;
}

bool Windows_Hook::StartHook(std::function<bool(bool)>& _key_combination_callback, std::set<ingame_overlay::ToggleKey> const& toggle_keys)
{
    if (!_Hooked)
    {
        if (!_key_combination_callback)
        {
            SPDLOG_ERROR("Failed to hook Windows: No key combination callback.");
            return false;
        }

        if (toggle_keys.empty())
        {
            SPDLOG_ERROR("Failed to hook Windows: No key combination.");
            return false;
        }

        void* hUser32 = System::Library::GetLibraryHandle(DLL_NAME);
        if (hUser32 == nullptr)
        {
            SPDLOG_WARN("Failed to hook Windows: Cannot find {}", DLL_NAME);
            return false;
        }

        System::Library::Library libUser32;
        LibraryName = System::Library::GetLibraryPath(hUser32);
        if (!libUser32.OpenLibrary(LibraryName, false))
        {
            SPDLOG_WARN("Failed to hook Windows: Cannot load {}", LibraryName);
            return false;
        }

        struct {
            void** func_ptr;
            void* hook_ptr;
            const char* func_name;
        } hook_array[] = {
            { (void**)&GetRawInputBuffer, &Windows_Hook::MyGetRawInputBuffer, "GetRawInputBuffer" },
            { (void**)&GetRawInputData  , &Windows_Hook::MyGetRawInputData  , "GetRawInputData"   },
            { (void**)&GetKeyState      , &Windows_Hook::MyGetKeyState      , "GetKeyState"       },
            { (void**)&GetAsyncKeyState , &Windows_Hook::MyGetAsyncKeyState , "GetAsyncKeyState"  },
            { (void**)&GetKeyboardState , &Windows_Hook::MyGetKeyboardState , "GetKeyboardState"  },
            { (void**)&GetCursorPos     , &Windows_Hook::MyGetCursorPos     , "GetCursorPos"      },
            { (void**)&SetCursorPos     , &Windows_Hook::MySetCursorPos     , "SetCursorPos"      },
            { (void**)&GetClipCursor    , &Windows_Hook::MyGetClipCursor    , "GetClipCursor"     },
            { (void**)&ClipCursor       , &Windows_Hook::MyClipCursor       , "ClipCursor"        },
        };

        for (auto& entry : hook_array)
        {
            *entry.func_ptr = libUser32.GetSymbol<void*>(entry.func_name);
            if (entry.func_ptr == nullptr)
            {
                SPDLOG_ERROR("Failed to hook Windows: Events function {} missing.", entry.func_name);
                return false;
            }
        }

        SPDLOG_INFO("Hooked Windows");
        _KeyCombinationCallback = std::move(_key_combination_callback);

        for (auto& key : toggle_keys)
        {
            uint32_t k = ToggleKeyToNativeKey(key);
            if (k != 0)
            {
                _NativeKeyCombination.insert(k);
            }
        }

        BeginHook();

        for (auto& entry : hook_array)
        {
            HookFunc(std::make_pair(entry.func_ptr, entry.hook_ptr));
        }

        EndHook();

        _Hooked = true;
    }
    return true;
}

void Windows_Hook::ResetRenderState()
{
    if (_Initialized)
    {
        _Initialized = false;
        SetWindowLongPtr(_GameHwnd, GWLP_WNDPROC, (LONG_PTR)_GameWndProc);
        _GameHwnd = nullptr;
        _GameWndProc = nullptr;
        ImGui_ImplWin32_Shutdown();
    }
}

void Windows_Hook::SetInitialWindowSize(HWND hWnd)
{
    RECT rect = { 0, 0, 0, 0 };
    ::GetClientRect(hWnd, &rect);
    ImGui::GetIO().DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
}

bool Windows_Hook::PrepareForOverlay(HWND hWnd)
{
    if (_GameHwnd != hWnd)
        ResetRenderState();

    if (!_Initialized)
    {
        _GameHwnd = hWnd;
        ImGui_ImplWin32_Init(_GameHwnd);

        _GameWndProc = (WNDPROC)SetWindowLongPtr(_GameHwnd, GWLP_WNDPROC, (LONG_PTR)&Windows_Hook::HookWndProc);
        _Initialized = true;
    }

    if (_Initialized)
    {
        void* current_proc = (void*)GetWindowLongPtr(_GameHwnd, GWLP_WNDPROC);
        if (current_proc == nullptr)
            return false;

        ImGui_ImplWin32_NewFrame();
        // Read keyboard modifiers inputs
        auto& io = ImGui::GetIO();

        POINT pos;
        if (this->GetCursorPos(&pos) && ScreenToClient(hWnd, &pos))
        {
            io.AddMousePosEvent((float)pos.x, (float)pos.y);
        }

        return true;
    }

    return false;
}

HWND Windows_Hook::GetGameHwnd() const
{
    return _GameHwnd;
}

WNDPROC Windows_Hook::GetGameWndProc() const
{
    return _GameWndProc;
}

/////////////////////////////////////////////////////////////////////////////////////
// Windows window hooks
bool IgnoreMsg(UINT uMsg)
{
    switch (uMsg)
    {
        // Mouse Events
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
        case WM_LBUTTONUP: case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONUP: case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONUP: case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
        case WM_XBUTTONUP: case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
        case WM_MOUSEACTIVATE: case WM_MOUSEHOVER: case WM_MOUSELEAVE:
        // Keyboard Events
        case WM_KEYDOWN: case WM_KEYUP:
        case WM_SYSKEYDOWN: case WM_SYSKEYUP: case WM_SYSDEADCHAR:
        case WM_CHAR: case WM_UNICHAR: case WM_DEADCHAR:
        // Raw Input Events
        case WM_INPUT:
            return true;
    }
    return false;
}

void RawEvent(RAWINPUT& raw)
{
    HWND hWnd = Windows_Hook::Inst()->GetGameHwnd();
    switch(raw.header.dwType)
    {
        case RIM_TYPEMOUSE:
            if (raw.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
                ImGui_ImplWin32_WndProcHandler(hWnd, WM_LBUTTONDOWN, 0, 0);
            if (raw.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
                ImGui_ImplWin32_WndProcHandler(hWnd, WM_LBUTTONUP, 0, 0);
            if (raw.data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
                ImGui_ImplWin32_WndProcHandler(hWnd, WM_RBUTTONDOWN, 0, 0);
            if (raw.data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
                ImGui_ImplWin32_WndProcHandler(hWnd, WM_RBUTTONUP, 0, 0);
            if (raw.data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
                ImGui_ImplWin32_WndProcHandler(hWnd, WM_MBUTTONDOWN, 0, 0);
            if (raw.data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
                ImGui_ImplWin32_WndProcHandler(hWnd, WM_MBUTTONUP, 0, 0);
            if (raw.data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
                ImGui_ImplWin32_WndProcHandler(hWnd, WM_MOUSEWHEEL, ((WPARAM)raw.data.mouse.usButtonData) << 16, 0);
            if (raw.data.mouse.usButtonFlags & RI_MOUSE_HWHEEL)
                ImGui_ImplWin32_WndProcHandler(hWnd, WM_MOUSEHWHEEL, ((WPARAM)raw.data.mouse.usButtonData) << 16, 0);
            break;

        //case RIM_TYPEKEYBOARD:
            //ImGui_ImplWin32_WndProcHandler(hWnd, raw.data.keyboard.Message, raw.data.keyboard.VKey, 0);
            //break;
    }
}

LRESULT CALLBACK Windows_Hook::HookWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Windows_Hook* inst = Windows_Hook::Inst();
    bool skip_input = inst->_KeyCombinationCallback(false);
    bool clean_keys = false;
    if (inst->_Initialized)
    {
        // Is the event is a key press
        if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN || uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP)
        {
            int key_count = 0;
            for (auto const& key : inst->_NativeKeyCombination)
            {
                if (inst->GetAsyncKeyState(key) & (1 << 15))
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
                        inst->GetCursorPos(&inst->_SavedCursorPos);
                        inst->GetClipCursor(&inst->_SavedClipCursor);
                    }
                    else
                    {
                        inst->ClipCursor(&inst->_SavedClipCursor);
                    }
                    inst->_KeyCombinationPushed = true;
                }
            }
            else
            {
                inst->_KeyCombinationPushed = false;
            }
        }

        if (skip_input && IgnoreMsg(uMsg))
        {
            ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
            if (clean_keys)
            {
                auto& io = ImGui::GetIO();
                io.ClearInputKeys();
                io.ClearInputCharacters();
            }
            return 0;
        }
    }

    // Protect against recursive call of the WindowProc...
    if (inst->_RecurseCallCount > 16)
        return 0;

    ++inst->_RecurseCallCount;
    // Call the overlay window procedure
    auto res = CallWindowProc(Windows_Hook::Inst()->_GameWndProc, hWnd, uMsg, wParam, lParam);
    --inst->_RecurseCallCount;
    return res;
}

UINT WINAPI Windows_Hook::MyGetRawInputBuffer(PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader)
{
    Windows_Hook* inst = Windows_Hook::Inst();
    int res = inst->GetRawInputBuffer(pData, pcbSize, cbSizeHeader);
    if (!inst->_Initialized)
        return res;

    if (pData != nullptr)
    {
        for (int i = 0; i < res; ++i)
            RawEvent(pData[i]);
    }

    if (!inst->_KeyCombinationCallback(false))
        return res;

    return 0;
}

UINT WINAPI Windows_Hook::MyGetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader)
{
    Windows_Hook* inst = Windows_Hook::Inst();
    auto res = inst->GetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
    if (!inst->_Initialized || pData == nullptr)
        return res;

    if (uiCommand == RID_INPUT && res == sizeof(RAWINPUT))
        RawEvent(*reinterpret_cast<RAWINPUT*>(pData));

    if (!inst->_KeyCombinationCallback(false))
        return res;

    memset(pData, 0, *pcbSize);
    *pcbSize = 0;
    return 0;
}

SHORT WINAPI Windows_Hook::MyGetKeyState(int nVirtKey)
{
    Windows_Hook* inst = Windows_Hook::Inst();

    if (inst->_Initialized && inst->_KeyCombinationCallback(false))
        return 0;

    return inst->GetKeyState(nVirtKey);
}

SHORT WINAPI Windows_Hook::MyGetAsyncKeyState(int vKey)
{
    Windows_Hook* inst = Windows_Hook::Inst();

    if (inst->_Initialized && inst->_KeyCombinationCallback(false))
        return 0;

    return inst->GetAsyncKeyState(vKey);
}

BOOL WINAPI Windows_Hook::MyGetKeyboardState(PBYTE lpKeyState)
{
    Windows_Hook* inst = Windows_Hook::Inst();

    if (inst->_Initialized && inst->_KeyCombinationCallback(false))
        return FALSE;

    return inst->GetKeyboardState(lpKeyState);
}

BOOL  WINAPI Windows_Hook::MyGetCursorPos(LPPOINT lpPoint)
{
    Windows_Hook* inst = Windows_Hook::Inst();
    
    BOOL res = inst->GetCursorPos(lpPoint);
    if (inst->_Initialized && inst->_KeyCombinationCallback(false) && lpPoint != nullptr)
    {
        *lpPoint = inst->_SavedCursorPos;
    }

    return res;
}

BOOL WINAPI Windows_Hook::MySetCursorPos(int X, int Y)
{
    Windows_Hook* inst = Windows_Hook::Inst();

    if (!inst->_Initialized || !inst->_KeyCombinationCallback(false))
        return inst->SetCursorPos(X, Y);

    return TRUE;
}

BOOL WINAPI Windows_Hook::MyGetClipCursor(RECT* lpRect)
{
    Windows_Hook* inst = Windows_Hook::Inst();
    if (lpRect == nullptr || !inst->_Initialized || !inst->_KeyCombinationCallback(false))
        return inst->GetClipCursor(lpRect);

    *lpRect = inst->_SavedClipCursor;
    return TRUE;
}

BOOL  WINAPI Windows_Hook::MyClipCursor(CONST RECT* lpRect)
{
    Windows_Hook* inst = Windows_Hook::Inst();
    CONST RECT* v = lpRect == nullptr ? &inst->_DefaultClipCursor : lpRect;

    inst->_SavedClipCursor = *v;

    if (!inst->_Initialized || !inst->_KeyCombinationCallback(false))
        return inst->ClipCursor(v);
    
    return inst->ClipCursor(&inst->_DefaultClipCursor);
}

/////////////////////////////////////////////////////////////////////////////////////

Windows_Hook::Windows_Hook() :
    _Initialized(false),
    _Hooked(false),
    _RecurseCallCount(0),
    _GameHwnd(nullptr),
    _GameWndProc(nullptr),
    _DefaultClipCursor{ LONG(0xFFFF8000), LONG(0xFFFF8000), LONG(0x00007FFF), LONG(0x00007FFF) },
    _KeyCombinationPushed(false)
{
}

Windows_Hook::~Windows_Hook()
{
    SPDLOG_INFO("Windows Hook removed");

    ResetRenderState();

    _inst = nullptr;
}

Windows_Hook* Windows_Hook::Inst()
{
    if (_inst == nullptr)
        _inst = new Windows_Hook;

    return _inst;
}

std::string Windows_Hook::GetLibraryName() const
{
    return LibraryName;
}