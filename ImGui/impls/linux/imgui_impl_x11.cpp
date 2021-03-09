// dear imgui: Platform Binding for Windows (standard windows API for 32 and 64 bits applications)
// This needs to be used along with a Renderer (e.g. DirectX11, OpenGL3, Vulkan..)

// Implemented features:
//  [?] Platform: Clipboard support
//  [?] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'.
//  [?] Platform: Keyboard arrays indexed using
//  [?] Platform: Gamepad support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.

#include "imgui.h"
#include "imgui_impl_x11.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glew.h>

#include <ctime>

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2019-08-31: Initial X11 implementation

// X11 Data
static Display*             g_Display = nullptr;
static Window               g_Window = 0;
static uint64_t             g_Time = 0;
static uint64_t             g_TicksPerSecond = 0;
static ImGuiMouseCursor     g_LastMouseCursor = ImGuiMouseCursor_COUNT;
static bool                 g_HasGamepad = false;
static bool                 g_WantUpdateHasGamepad = true;

bool GetKeyState(int keysym, char keys[32])
{
    int keycode = XKeysymToKeycode(g_Display, keysym);
    return keys[keycode/8] & (1<<keycode%8);
}

bool IsKeySys(int key)
{
    switch(key)
    {
        case XK_Shift_L  : case XK_Shift_R   :
        case XK_Control_L: case XK_Control_R :
        case XK_Alt_L    : case XK_Alt_R     :
        case XK_Super_L  : case XK_Super_R   :
        case XK_Caps_Lock: case XK_Shift_Lock:
        case XK_BackSpace: case XK_Delete    :
        case XK_Left     : case XK_Right     :
        case XK_Up       : case XK_Down      :
        case XK_Prior    : case XK_Next      :
        case XK_Home     : case XK_End       :
        case XK_Insert   : case XK_Return    :
            return true;
    }
    return false;
}

// Functions
bool    ImGui_ImplX11_Init(void *display, void *window)
{
    timespec ts, tsres;
    clock_getres(CLOCK_MONOTONIC_RAW, &tsres);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);

    g_TicksPerSecond = 1000000000.0f / (static_cast<uint64_t>(tsres.tv_nsec) + static_cast<uint64_t>(tsres.tv_sec)*1000000000);
    g_Time = static_cast<uint64_t>(ts.tv_nsec) + static_cast<uint64_t>(ts.tv_sec)*1000000000;

    //if (!::QueryPerformanceFrequency((LARGE_INTEGER *)&g_TicksPerSecond))
    //    return false;
    //if (!::QueryPerformanceCounter((LARGE_INTEGER *)&g_Time))
    //    return false;

    // Setup back-end capabilities flags
    g_Display = reinterpret_cast<Display*>(display);
    g_Window = reinterpret_cast<Window>(window);
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendPlatformName = "imgui_impl_x11";
    io.ImeWindowHandle = nullptr;

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_Tab]        = XKeysymToKeycode(g_Display, XK_Tab);
    io.KeyMap[ImGuiKey_LeftArrow]  = XKeysymToKeycode(g_Display, XK_Left);
    io.KeyMap[ImGuiKey_RightArrow] = XKeysymToKeycode(g_Display, XK_Right);
    io.KeyMap[ImGuiKey_UpArrow]    = XKeysymToKeycode(g_Display, XK_Up);
    io.KeyMap[ImGuiKey_DownArrow]  = XKeysymToKeycode(g_Display, XK_Down);
    io.KeyMap[ImGuiKey_PageUp]     = XKeysymToKeycode(g_Display, XK_Prior);
    io.KeyMap[ImGuiKey_PageDown]   = XKeysymToKeycode(g_Display, XK_Next);
    io.KeyMap[ImGuiKey_Home]       = XKeysymToKeycode(g_Display, XK_Home);
    io.KeyMap[ImGuiKey_End]        = XKeysymToKeycode(g_Display, XK_End);
    io.KeyMap[ImGuiKey_Insert]     = XKeysymToKeycode(g_Display, XK_Insert);
    io.KeyMap[ImGuiKey_Delete]     = XKeysymToKeycode(g_Display, XK_Delete);
    io.KeyMap[ImGuiKey_Backspace]  = XKeysymToKeycode(g_Display, XK_BackSpace);
    io.KeyMap[ImGuiKey_Space]      = XKeysymToKeycode(g_Display, XK_space);
    io.KeyMap[ImGuiKey_Enter]      = XKeysymToKeycode(g_Display, XK_Return);
    io.KeyMap[ImGuiKey_Escape]     = XKeysymToKeycode(g_Display, XK_Escape);
    io.KeyMap[ImGuiKey_A]          = XKeysymToKeycode(g_Display, XK_A);
    io.KeyMap[ImGuiKey_C]          = XKeysymToKeycode(g_Display, XK_C);
    io.KeyMap[ImGuiKey_V]          = XKeysymToKeycode(g_Display, XK_V);
    io.KeyMap[ImGuiKey_X]          = XKeysymToKeycode(g_Display, XK_X);
    io.KeyMap[ImGuiKey_Y]          = XKeysymToKeycode(g_Display, XK_Y);
    io.KeyMap[ImGuiKey_Z]          = XKeysymToKeycode(g_Display, XK_Z);

    return true;
}

void    ImGui_ImplX11_Shutdown()
{
    g_Display = nullptr;
    g_Window = 0;
}

static bool ImGui_ImplX11_UpdateMouseCursor()
{
    /*
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return false;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        ::SetCursor(NULL);
    }
    else
    {
        // Show OS mouse cursor
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor)
        {
        case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
        case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
        case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
        case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
        case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
        case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
        case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
        case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
        }
        ::SetCursor(::LoadCursor(NULL, win32_cursor));
    }
    */
    return true;
}

static void ImGui_ImplX11_UpdateMousePos()
{
    ImGuiIO& io = ImGui::GetIO();

    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if (io.WantSetMousePos)
    {
    //    POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
    //    ::ClientToScreen(g_hWnd, &pos);
    //    ::SetCursorPos(pos.x, pos.y);
    }

    // Set mouse position
    Window unused_window;
    int rx, ry, x, y;
    unsigned int mask;

    XQueryPointer(g_Display, g_Window, &unused_window, &unused_window, &rx, &ry, &x, &y, &mask);

    io.MousePos = ImVec2((float)x, (float)y);
}

// Gamepad navigation mapping
static void ImGui_ImplX11_UpdateGamepads()
{
    /* TODO: support linux gamepad ?
    ImGuiIO& io = ImGui::GetIO();
    memset(io.NavInputs, 0, sizeof(io.NavInputs));
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return;

    // Calling XInputGetState() every frame on disconnected gamepads is unfortunately too slow.
    // Instead we refresh gamepad availability by calling XInputGetCapabilities() _only_ after receiving WM_DEVICECHANGE.
    if (g_WantUpdateHasGamepad)
    {
        XINPUT_CAPABILITIES caps;
        g_HasGamepad = (XInputGetCapabilities(0, XINPUT_FLAG_GAMEPAD, &caps) == ERROR_SUCCESS);
        g_WantUpdateHasGamepad = false;
    }

    XINPUT_STATE xinput_state;
    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    if (g_HasGamepad && XInputGetState(0, &xinput_state) == ERROR_SUCCESS)
    {
        const XINPUT_GAMEPAD& gamepad = xinput_state.Gamepad;
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

        #define MAP_BUTTON(NAV_NO, BUTTON_ENUM)     { io.NavInputs[NAV_NO] = (gamepad.wButtons & BUTTON_ENUM) ? 1.0f : 0.0f; }
        #define MAP_ANALOG(NAV_NO, VALUE, V0, V1)   { float vn = (float)(VALUE - V0) / (float)(V1 - V0); if (vn > 1.0f) vn = 1.0f; if (vn > 0.0f && io.NavInputs[NAV_NO] < vn) io.NavInputs[NAV_NO] = vn; }
        MAP_BUTTON(ImGuiNavInput_Activate,      XINPUT_GAMEPAD_A);              // Cross / A
        MAP_BUTTON(ImGuiNavInput_Cancel,        XINPUT_GAMEPAD_B);              // Circle / B
        MAP_BUTTON(ImGuiNavInput_Menu,          XINPUT_GAMEPAD_X);              // Square / X
        MAP_BUTTON(ImGuiNavInput_Input,         XINPUT_GAMEPAD_Y);              // Triangle / Y
        MAP_BUTTON(ImGuiNavInput_DpadLeft,      XINPUT_GAMEPAD_DPAD_LEFT);      // D-Pad Left
        MAP_BUTTON(ImGuiNavInput_DpadRight,     XINPUT_GAMEPAD_DPAD_RIGHT);     // D-Pad Right
        MAP_BUTTON(ImGuiNavInput_DpadUp,        XINPUT_GAMEPAD_DPAD_UP);        // D-Pad Up
        MAP_BUTTON(ImGuiNavInput_DpadDown,      XINPUT_GAMEPAD_DPAD_DOWN);      // D-Pad Down
        MAP_BUTTON(ImGuiNavInput_FocusPrev,     XINPUT_GAMEPAD_LEFT_SHOULDER);  // L1 / LB
        MAP_BUTTON(ImGuiNavInput_FocusNext,     XINPUT_GAMEPAD_RIGHT_SHOULDER); // R1 / RB
        MAP_BUTTON(ImGuiNavInput_TweakSlow,     XINPUT_GAMEPAD_LEFT_SHOULDER);  // L1 / LB
        MAP_BUTTON(ImGuiNavInput_TweakFast,     XINPUT_GAMEPAD_RIGHT_SHOULDER); // R1 / RB
        MAP_ANALOG(ImGuiNavInput_LStickLeft,    gamepad.sThumbLX,  -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32768);
        MAP_ANALOG(ImGuiNavInput_LStickRight,   gamepad.sThumbLX,  +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
        MAP_ANALOG(ImGuiNavInput_LStickUp,      gamepad.sThumbLY,  +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, +32767);
        MAP_ANALOG(ImGuiNavInput_LStickDown,    gamepad.sThumbLY,  -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, -32767);
        #undef MAP_BUTTON
        #undef MAP_ANALOG
    }
    */
}

void    ImGui_ImplX11_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    unsigned int width, height;
    Window unused_window;
    int unused_int;
    unsigned int unused_unsigned_int;

    XGetGeometry(g_Display, (Window)g_Window, &unused_window, &unused_int, &unused_int, &width, &height, &unused_unsigned_int, &unused_unsigned_int);

    io.DisplaySize.x = width;
    io.DisplaySize.y = height;

    timespec ts, tsres;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);

    uint64_t current_time = static_cast<uint64_t>(ts.tv_nsec) + static_cast<uint64_t>(ts.tv_sec)*1000000000;

    io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
    g_Time = current_time;

    // Read keyboard modifiers inputs
    char keys[32];
    XQueryKeymap(g_Display, keys);

    io.KeyCtrl = GetKeyState(XK_Control_L, keys);
    io.KeyShift = GetKeyState(XK_Shift_L, keys);
    io.KeyAlt = GetKeyState(XK_Alt_L, keys);
    io.KeySuper = false;
    // io.KeysDown[], io.MousePos, io.MouseDown[], io.MouseWheel: filled by the WndProc handler below.

    // Update OS mouse position
    ImGui_ImplX11_UpdateMousePos();
    /*
    // Update OS mouse cursor with the cursor requested by imgui
    ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
    if (g_LastMouseCursor != mouse_cursor)
    {
        g_LastMouseCursor = mouse_cursor;
        ImGui_ImplX11_UpdateMouseCursor();
    }
    */

    // Update game controllers (if enabled and available)
    ImGui_ImplX11_UpdateGamepads();
}

// Process X11 mouse/keyboard inputs.
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
IMGUI_IMPL_API int ImGui_ImplX11_EventHandler(XEvent &event)
{
    if (ImGui::GetCurrentContext() == NULL)
        return 0;

    ImGuiIO& io = ImGui::GetIO();
    switch (event.type)
    {
        case ButtonPress:
        case ButtonRelease:
            switch(event.xbutton.button)
            {
                case Button1:
                    io.MouseDown[0] = event.type == ButtonPress;
                    break;

                case Button2:
                    io.MouseDown[2] = event.type == ButtonPress;
                    break;

                case Button3:
                    io.MouseDown[1] = event.type == ButtonPress;
                    break;

                case Button4: // Mouse wheel up
                    if( event.type == ButtonPress )
                        io.MouseWheel += 1;
                    return 0;

                case Button5: // Mouse wheel down
                    if( event.type == ButtonPress )
                        io.MouseWheel -= 1;
                    return 0;
            }

            break;

        case KeyPress:
        {
            int key = XKeycodeToKeysym(g_Display, event.xkey.keycode, event.xkey.state & ShiftMask ? 1 : 0);
            if( IsKeySys(key) )
                io.KeysDown[event.xkey.keycode] = true;
            else
                io.AddInputCharacter(key);
            return 0;
        }

        case KeyRelease:
        {
            int key = XKeycodeToKeysym(g_Display, event.xkey.keycode, event.xkey.state & ShiftMask ? 1 : 0);
            if( IsKeySys(key) )
                io.KeysDown[event.xkey.keycode] = false;
            return 0;
        }
    }
            /*
    case WM_DEVICECHANGE:
        if ((UINT)wParam == DBT_DEVNODES_CHANGED)
            g_WantUpdateHasGamepad = true;
        return 0;
    }
    */
    return 0;
}

