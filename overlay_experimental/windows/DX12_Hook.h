#ifndef __INCLUDED_DX12_HOOK_H__
#define __INCLUDED_DX12_HOOK_H__

#include "../Base_Hook.h"
#ifdef EMU_OVERLAY

#include <d3d12.h>
#include <dxgi1_4.h>
#include "DirectX_VTables.h"

class DX12_Hook : public Base_Hook
{
public:
    #define DX12_DLL "d3d12.dll"

private:
    static DX12_Hook* _inst;

    // Variables
    bool hooked;
    bool initialized;

    ID3D12CommandQueue* pCmdQueue;
    UINT bufferCount;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> mainRenderTargets;
    ID3D12CommandAllocator** pCmdAlloc;
    ID3D12DescriptorHeap* pSrvDescHeap;
    ID3D12GraphicsCommandList* pCmdList;
    ID3D12DescriptorHeap* pRtvDescHeap;
    ID3D12Resource** pBackBuffer;

    // Functions
    DX12_Hook();

    void resetRenderState();
    void prepareForOverlay(IDXGISwapChain* pSwapChain);

    // Hook to render functions
    static HRESULT STDMETHODCALLTYPE MyPresent(IDXGISwapChain* _this, UINT SyncInterval, UINT Flags);
    static HRESULT STDMETHODCALLTYPE MyResizeTarget(IDXGISwapChain* _this, const DXGI_MODE_DESC* pNewTargetParameters);
    static HRESULT STDMETHODCALLTYPE MyResizeBuffers(IDXGISwapChain* _this, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
    static void STDMETHODCALLTYPE MyExecuteCommandLists(ID3D12CommandQueue *_this, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);

    decltype(&IDXGISwapChain::Present)       Present;
    decltype(&IDXGISwapChain::ResizeBuffers) ResizeBuffers;
    decltype(&IDXGISwapChain::ResizeTarget)  ResizeTarget;
    decltype(&ID3D12CommandQueue::ExecuteCommandLists) ExecuteCommandLists;

public:
    virtual ~DX12_Hook();

    bool start_hook();
    static DX12_Hook* Inst();
    virtual const char* get_lib_name() const;

    void loadFunctions(ID3D12CommandQueue* pCommandQueue, IDXGISwapChain* pSwapChain);
};

#endif//EMU_OVERLAY
#endif//__INCLUDED_DX12_HOOK_H__
