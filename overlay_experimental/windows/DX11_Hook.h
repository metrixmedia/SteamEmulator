#ifndef __INCLUDED_DX11_HOOK_H__
#define __INCLUDED_DX11_HOOK_H__

#include "../Base_Hook.h"
#ifdef EMU_OVERLAY

#include <d3d11.h>
#include "DirectX_VTables.h"

class DX11_Hook : public Base_Hook
{
public:
    #define DX11_DLL "d3d11.dll"

private:
    static DX11_Hook* _inst;

    // Variables
    bool hooked;
    bool initialized;
    ID3D11DeviceContext* pContext;
    ID3D11RenderTargetView* mainRenderTargetView;

    // Functions
    DX11_Hook();

    void resetRenderState();
    void prepareForOverlay(IDXGISwapChain* pSwapChain);

    // Hook to render functions
    static HRESULT STDMETHODCALLTYPE MyPresent(IDXGISwapChain* _this, UINT SyncInterval, UINT Flags);
    static HRESULT STDMETHODCALLTYPE MyResizeTarget(IDXGISwapChain* _this, const DXGI_MODE_DESC* pNewTargetParameters);
    static HRESULT STDMETHODCALLTYPE MyResizeBuffers(IDXGISwapChain* _this, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

    decltype(&IDXGISwapChain::Present)       Present;
    decltype(&IDXGISwapChain::ResizeBuffers) ResizeBuffers;
    decltype(&IDXGISwapChain::ResizeTarget)  ResizeTarget;

public:
    virtual ~DX11_Hook();

    bool start_hook();
    static DX11_Hook* Inst();
    virtual const char* get_lib_name() const;

    void loadFunctions(IDXGISwapChain *pSwapChain);
};

#endif//EMU_OVERLAY

#endif//__INCLUDED_DX11_HOOK_H__
