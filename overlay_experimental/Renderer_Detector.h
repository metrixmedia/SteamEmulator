#ifndef __INCLUDED_RENDERER_DETECTOR_H__
#define __INCLUDED_RENDERER_DETECTOR_H__

#include "Base_Hook.h"

#ifdef EMU_OVERLAY
#ifdef __WINDOWS__

struct IDXGISwapChain;
struct IDirect3DDevice9;
struct IDirect3DDevice9Ex;

class Renderer_Detector
{
private:
    // Variables
    std::thread* _hook_thread;
    std::mutex _found_mutex;
    unsigned int _hook_retries;
    bool _renderer_found;       // Is the renderer hooked ?
    bool _dx9_hooked;
    bool _dx10_hooked;
    bool _dx11_hooked;
    bool _dx12_hooked;
    bool _dxgi_hooked;
    bool _ogl_hooked;           // wglMakeCurrent is hooked ? (opengl)
    Base_Hook* rendererdetect_hook;
    Base_Hook* game_renderer;

    ATOM atom;
    HWND dummy_hWnd;

    // Functions
    Renderer_Detector();
    ~Renderer_Detector();

    static HRESULT STDMETHODCALLTYPE MyIDXGISwapChain_Present(IDXGISwapChain* _this, UINT SyncInterval, UINT Flags);
    static HRESULT STDMETHODCALLTYPE MyPresent(IDirect3DDevice9* _this, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
    static HRESULT STDMETHODCALLTYPE MyPresentEx(IDirect3DDevice9Ex* _this, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags);
    static BOOL WINAPI MywglMakeCurrent(HDC hDC, HGLRC hGLRC);

    void HookDXGIPresent(IDXGISwapChain* pSwapChain);
    void HookDX9Present(IDirect3DDevice9* pDevice, bool ex);
    void HookwglMakeCurrent(decltype(wglMakeCurrent)* wglMakeCurrent);

    void hook_dx9();
    void hook_dx10();
    void hook_dx11();
    void hook_dx12();
    void hook_opengl();

    void create_hwnd();
    void destroy_hwnd();
    void create_hook(const char* libname);
    bool stop_retry();

    static void find_renderer_proc(Renderer_Detector* _this);

public:
    void find_renderer();
    void renderer_found(Base_Hook* hook);
    Base_Hook* get_renderer() const;
    static Renderer_Detector& Inst();
};

#elif defined __LINUX__
#include "linux/OpenGLX_Hook.h"

class Renderer_Detector
{
    // Variables
    std::thread* _hook_thread;
    std::mutex _found_mutex;
    unsigned int _hook_retries;
    bool _oglx_hooked;
    bool _renderer_found;       // Is the renderer hooked ?
    Base_Hook* rendererdetect_hook;
    Base_Hook* game_renderer;

    // Functions
    Renderer_Detector();
    ~Renderer_Detector();

    static void MyglXSwapBuffers(Display *dpy, GLXDrawable drawable);

    void HookglXSwapBuffers(decltype(glXSwapBuffers)* glXSwapBuffers);

    void hook_openglx(const char* libname);

    void create_hook(const char* libname);
    bool stop_retry();

    static void find_renderer_proc(Renderer_Detector* _this);

public:
    void find_renderer();
    void renderer_found(Base_Hook* hook);
    Base_Hook* get_renderer() const;
    static Renderer_Detector& Inst();
};

#endif//__WINDOWS__
#endif//EMU_OVERLAY
#endif//__INCLUDED_RENDERER_DETECTOR_H__
