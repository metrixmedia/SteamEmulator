#ifndef __INCLUDED_WINDOWS_HOOK_H__
#define __INCLUDED_WINDOWS_HOOK_H__

#include "../Base_Hook.h"

#ifdef __WINDOWS__
#ifdef EMU_OVERLAY

class Windows_Hook : public Base_Hook
{
public:
    #define WINDOWS_DLL "user32.dll"

private:
    static Windows_Hook* _inst;

    // Variables
    bool hooked;
    bool initialized;
    HWND _game_hwnd;
    WNDPROC _game_wndproc;

    // Functions
    Windows_Hook();

    // Hook to Windows window messages
    decltype(GetRawInputBuffer)* GetRawInputBuffer;
    decltype(GetRawInputData)* GetRawInputData;
    decltype(SetCursorPos)* SetCursorPos;

    static LRESULT CALLBACK HookWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static UINT WINAPI MyGetRawInputBuffer(PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader);
    static UINT WINAPI MyGetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader);

    static BOOL WINAPI MySetCursorPos(int x, int y);
public:
    virtual ~Windows_Hook();

    void resetRenderState();
    void prepareForOverlay(HWND);

    HWND GetGameHwnd() const;
    WNDPROC GetGameWndProc() const;

    bool start_hook();
    static Windows_Hook* Inst();
    virtual const char* get_lib_name() const;
};

#endif//EMU_OVERLAY
#endif//__WINDOWS__
#endif//__INCLUDED_WINDOWS_HOOK_H__
