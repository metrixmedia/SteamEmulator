#include "Windows_Hook.h"
#include "../Renderer_Detector.h"
#include "../../dll/dll.h"

#ifdef EMU_OVERLAY

#include <imgui.h>
#include <impls/windows/imgui_impl_win32.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Windows_Hook* Windows_Hook::_inst = nullptr;

bool Windows_Hook::start_hook()
{
    bool res = true;
    if (!hooked)
    {
        GetRawInputBuffer = ::GetRawInputBuffer;
        GetRawInputData = ::GetRawInputData;
        SetCursorPos = ::SetCursorPos;

        PRINT_DEBUG("Hooked Windows\n");

        BeginHook();
        HookFuncs(
            std::make_pair<void**, void*>(&(PVOID&)GetRawInputBuffer, &Windows_Hook::MyGetRawInputBuffer),
            std::make_pair<void**, void*>(&(PVOID&)GetRawInputData  , &Windows_Hook::MyGetRawInputData),
            std::make_pair<void**, void*>(&(PVOID&)SetCursorPos  , &Windows_Hook::MySetCursorPos)
        );
        EndHook();

        hooked = true;
    }
    return res;
}

void Windows_Hook::resetRenderState()
{
    if (initialized)
    {
        initialized = false;
        SetWindowLongPtr(_game_hwnd, GWLP_WNDPROC, (LONG_PTR)_game_wndproc);
        _game_hwnd = nullptr;
        _game_wndproc = nullptr;
        ImGui_ImplWin32_Shutdown();
    }
}

void Windows_Hook::prepareForOverlay(HWND hWnd)
{
    if (!initialized)
    {
        ImGui_ImplWin32_Init(hWnd);

        _game_hwnd = hWnd;
        _game_wndproc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)&Windows_Hook::HookWndProc);

        initialized = true;
    }

    ImGui_ImplWin32_NewFrame();
}

HWND Windows_Hook::GetGameHwnd() const
{
    return _game_hwnd;
}

WNDPROC Windows_Hook::GetGameWndProc() const
{
    return _game_wndproc;
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

void RawMouseEvent(RAWINPUT& raw)
{
    if (raw.header.dwType == RIM_TYPEMOUSE)
    {
        if (raw.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
            ImGui_ImplWin32_WndProcHandler(Windows_Hook::Inst()->GetGameHwnd(), WM_LBUTTONDOWN, 0, 0);
        else if (raw.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
            ImGui_ImplWin32_WndProcHandler(Windows_Hook::Inst()->GetGameHwnd(), WM_LBUTTONUP, 0, 0);
        else if (raw.data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
            ImGui_ImplWin32_WndProcHandler(Windows_Hook::Inst()->GetGameHwnd(), WM_MBUTTONDOWN, 0, 0);
        else if (raw.data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
            ImGui_ImplWin32_WndProcHandler(Windows_Hook::Inst()->GetGameHwnd(), WM_MBUTTONUP, 0, 0);
        else if (raw.data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
            ImGui_ImplWin32_WndProcHandler(Windows_Hook::Inst()->GetGameHwnd(), WM_RBUTTONDOWN, 0, 0);
        else if (raw.data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
            ImGui_ImplWin32_WndProcHandler(Windows_Hook::Inst()->GetGameHwnd(), WM_RBUTTONUP, 0, 0);
    }
}

LRESULT CALLBACK Windows_Hook::HookWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Steam_Overlay* overlay = get_steam_client()->steam_overlay;
    bool show = overlay->ShowOverlay();
    // Is the event is a key press
    if (uMsg == WM_KEYDOWN)
    {
        // Tab is pressed and was not pressed before
        if (wParam == VK_TAB && !(lParam & (1 << 30)))
        {
            // If Left Shift is pressed
            if (GetAsyncKeyState(VK_LSHIFT) & (1 << 15))
            {
                show = !overlay->ShowOverlay();
                overlay->ShowOverlay(show);
            }
        }
    }

    ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
    if (show)
    {
        if (IgnoreMsg(uMsg))
            return 0;
    }

    // Call the overlay window procedure
    return CallWindowProc(Windows_Hook::Inst()->_game_wndproc, hWnd, uMsg, wParam, lParam);
}

UINT WINAPI Windows_Hook::MyGetRawInputBuffer(PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader)
{
    if (pData == nullptr || !get_steam_client()->steam_overlay->ShowOverlay())
        return Windows_Hook::Inst()->GetRawInputBuffer(pData, pcbSize, cbSizeHeader);

    int num = Windows_Hook::Inst()->GetRawInputBuffer(pData, pcbSize, cbSizeHeader);
    for (int i = 0; i < num; ++i)
        RawMouseEvent(pData[i]);

    return 0;
}

UINT WINAPI Windows_Hook::MyGetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader)
{
    if (pData == nullptr || !get_steam_client()->steam_overlay->ShowOverlay())
        return Windows_Hook::Inst()->GetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

    Windows_Hook::Inst()->GetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

    RawMouseEvent(*reinterpret_cast<RAWINPUT*>(pData));

    *pcbSize = 0;

    return 0;
}

BOOL WINAPI Windows_Hook::MySetCursorPos(int x, int y)
{
    if (get_steam_client()->steam_overlay->ShowOverlay()) {
        POINT p;
        GetCursorPos(&p);
        x = p.x;
        y = p.y;
    }

    return Windows_Hook::Inst()->SetCursorPos(x, y);
}
/////////////////////////////////////////////////////////////////////////////////////

Windows_Hook::Windows_Hook() :
    initialized(false),
    hooked(false),
    _game_hwnd(nullptr),
    _game_wndproc(nullptr),
    GetRawInputBuffer(nullptr),
    GetRawInputData(nullptr)
{
    //_library = LoadLibrary(DLL_NAME);
}

Windows_Hook::~Windows_Hook()
{
    PRINT_DEBUG("Windows Hook removed\n");

    resetRenderState();

    //FreeLibrary(reinterpret_cast<HMODULE>(_library));

    _inst = nullptr;
}

Windows_Hook* Windows_Hook::Inst()
{
    if (_inst == nullptr)
        _inst = new Windows_Hook;

    return _inst;
}

const char* Windows_Hook::get_lib_name() const
{
    return WINDOWS_DLL;
}

#endif//EMU_OVERLAY