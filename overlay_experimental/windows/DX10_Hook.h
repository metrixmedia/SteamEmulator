#ifndef __INCLUDED_DX10_HOOK_H__
#define __INCLUDED_DX10_HOOK_H__

#include "../Base_Hook.h"
#ifdef EMU_OVERLAY

#include <d3d10.h>
#include "DirectX_VTables.h"

class DX10_Hook : public Base_Hook
{
public:
    #define DX10_DLL "d3d10.dll"

private:
    static DX10_Hook* _inst;

    // Variables
    bool hooked;
    bool initialized;
    ID3D10Device* pDevice;
    ID3D10RenderTargetView* mainRenderTargetView;

    // Functions
    DX10_Hook();

    void resetRenderState();
    void prepareForOverlay(IDXGISwapChain *pSwapChain);

    // Hook to render functions
    static HRESULT STDMETHODCALLTYPE MyPresent(IDXGISwapChain* _this, UINT SyncInterval, UINT Flags);
    static HRESULT STDMETHODCALLTYPE MyResizeTarget(IDXGISwapChain* _this, const DXGI_MODE_DESC* pNewTargetParameters);
    static HRESULT STDMETHODCALLTYPE MyResizeBuffers(IDXGISwapChain* _this, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

    decltype(&IDXGISwapChain::Present)       Present;
    decltype(&IDXGISwapChain::ResizeBuffers) ResizeBuffers;
    decltype(&IDXGISwapChain::ResizeTarget)  ResizeTarget;

public:
    virtual ~DX10_Hook();

    bool start_hook();
    static DX10_Hook* Inst();
    virtual const char* get_lib_name() const;

    void loadFunctions(IDXGISwapChain *pSwapChain);
};

#endif//EMU_OVERLAY

#endif//__INCLUDED_DX10_HOOK_H__
