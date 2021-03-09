#include "DX12_Hook.h"
#include "Windows_Hook.h"
#include "../Renderer_Detector.h"
#include "../../dll/dll.h"

#ifdef EMU_OVERLAY

#include <imgui.h>
#include <impls/windows/imgui_impl_dx12.h>

#include <dxgi1_4.h>

DX12_Hook* DX12_Hook::_inst = nullptr;

bool DX12_Hook::start_hook()
{
    bool res = true;
    if (!hooked)
    {
        if (!Windows_Hook::Inst()->start_hook())
            return false;

        PRINT_DEBUG("Hooked DirectX 12\n");
        hooked = true;

        Renderer_Detector::Inst().renderer_found(this);

        BeginHook();
        HookFuncs(
            std::make_pair<void**, void*>(&(PVOID&)DX12_Hook::Present, &DX12_Hook::MyPresent),
            std::make_pair<void**, void*>(&(PVOID&)DX12_Hook::ResizeTarget, &DX12_Hook::MyResizeTarget),
            std::make_pair<void**, void*>(&(PVOID&)DX12_Hook::ResizeBuffers, &DX12_Hook::MyResizeBuffers),
            std::make_pair<void**, void*>(&(PVOID&)DX12_Hook::ExecuteCommandLists, &DX12_Hook::MyExecuteCommandLists)
        );
        EndHook();

        get_steam_client()->steam_overlay->HookReady();
    }
    return res;
}

void DX12_Hook::resetRenderState()
{
    if (initialized)
    {
        pSrvDescHeap->Release();
        for (UINT i = 0; i < bufferCount; ++i)
        {
            pCmdAlloc[i]->Release();
            pBackBuffer[i]->Release();
        }
        pRtvDescHeap->Release();
        delete[]pCmdAlloc;
        delete[]pBackBuffer;

        ImGui_ImplDX12_Shutdown();
        Windows_Hook::Inst()->resetRenderState();
        ImGui::DestroyContext();

        initialized = false;
    }
}

// Try to make this function and overlay's proc as short as possible or it might affect game's fps.
void DX12_Hook::prepareForOverlay(IDXGISwapChain* pSwapChain)
{
    if (pCmdQueue == nullptr)
        return;

    ID3D12CommandQueue* pCmdQueue = this->pCmdQueue;

    IDXGISwapChain3* pSwapChain3 = nullptr;
    DXGI_SWAP_CHAIN_DESC sc_desc;
    pSwapChain->QueryInterface(IID_PPV_ARGS(&pSwapChain3));
    if (pSwapChain3 == nullptr)
        return;

    pSwapChain3->GetDesc(&sc_desc);

    if (!initialized)
    {
        UINT bufferIndex = pSwapChain3->GetCurrentBackBufferIndex();
        ID3D12Device* pDevice;
        if (pSwapChain3->GetDevice(IID_PPV_ARGS(&pDevice)) != S_OK)
            return;

        bufferCount = sc_desc.BufferCount;

        mainRenderTargets.clear();

        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = 1;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            if (pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pSrvDescHeap)) != S_OK)
            {
                pDevice->Release();
                pSwapChain3->Release();
                return;
            }
        }
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            desc.NumDescriptors = bufferCount;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            desc.NodeMask = 1;
            if (pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pRtvDescHeap)) != S_OK)
            {
                pDevice->Release();
                pSwapChain3->Release();
                pSrvDescHeap->Release();
                return;
            }
        
            SIZE_T rtvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
            pCmdAlloc = new ID3D12CommandAllocator * [bufferCount];
            for (int i = 0; i < bufferCount; ++i)
            {
                mainRenderTargets.push_back(rtvHandle);
                rtvHandle.ptr += rtvDescriptorSize;
            }
        }
        
        for (UINT i = 0; i < sc_desc.BufferCount; ++i)
        {
            if (pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCmdAlloc[i])) != S_OK)
            {
                pDevice->Release();
                pSwapChain3->Release();
                pSrvDescHeap->Release();
                for (UINT j = 0; j < i; ++j)
                {
                    pCmdAlloc[j]->Release();
                }
                pRtvDescHeap->Release();
                delete[]pCmdAlloc;
                return;
            }
        }

        if (pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCmdAlloc[0], NULL, IID_PPV_ARGS(&pCmdList)) != S_OK ||
            pCmdList->Close() != S_OK)
        {
            pDevice->Release();
            pSwapChain3->Release();
            pSrvDescHeap->Release();
            for (UINT i = 0; i < bufferCount; ++i)
                pCmdAlloc[i]->Release();
            pRtvDescHeap->Release();
            delete[]pCmdAlloc;
            return;
        }

        pBackBuffer = new ID3D12Resource * [bufferCount];
        for (UINT i = 0; i < bufferCount; i++)
        {
            pSwapChain3->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer[i]));
            pDevice->CreateRenderTargetView(pBackBuffer[i], NULL, mainRenderTargets[i]);
        }

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL;
    
        ImGui_ImplDX12_Init(pDevice, bufferCount, DXGI_FORMAT_R8G8B8A8_UNORM, NULL,
            pSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
            pSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
        
        get_steam_client()->steam_overlay->CreateFonts();

        initialized = true;

        pDevice->Release();
    }
    
    if (ImGui_ImplDX12_NewFrame())
    {
        Windows_Hook::Inst()->prepareForOverlay(sc_desc.OutputWindow);

        ImGui::NewFrame();

        get_steam_client()->steam_overlay->OverlayProc();

        UINT bufferIndex = pSwapChain3->GetCurrentBackBufferIndex();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = pBackBuffer[bufferIndex];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        pCmdAlloc[bufferIndex]->Reset();
        pCmdList->Reset(pCmdAlloc[bufferIndex], NULL);
        pCmdList->ResourceBarrier(1, &barrier);
        pCmdList->OMSetRenderTargets(1, &mainRenderTargets[bufferIndex], FALSE, NULL);
        pCmdList->SetDescriptorHeaps(1, &pSrvDescHeap);

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pCmdList);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        pCmdList->ResourceBarrier(1, &barrier);
        pCmdList->Close();

        pCmdQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&pCmdList);
    }
    pSwapChain3->Release();
}

HRESULT STDMETHODCALLTYPE DX12_Hook::MyPresent(IDXGISwapChain *_this, UINT SyncInterval, UINT Flags)
{
    DX12_Hook::Inst()->prepareForOverlay(_this);

    return (_this->*DX12_Hook::Inst()->Present)(SyncInterval, Flags);
}

HRESULT STDMETHODCALLTYPE DX12_Hook::MyResizeTarget(IDXGISwapChain* _this, const DXGI_MODE_DESC* pNewTargetParameters)
{
    DX12_Hook::Inst()->resetRenderState();
    return (_this->*DX12_Hook::Inst()->ResizeTarget)(pNewTargetParameters);
}

HRESULT STDMETHODCALLTYPE DX12_Hook::MyResizeBuffers(IDXGISwapChain* _this, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    DX12_Hook::Inst()->resetRenderState();
    return (_this->*DX12_Hook::Inst()->ResizeBuffers)(BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

void STDMETHODCALLTYPE DX12_Hook::MyExecuteCommandLists(ID3D12CommandQueue *_this, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists)
{
    DX12_Hook* me = DX12_Hook::Inst();
    me->pCmdQueue = _this;

    (_this->*DX12_Hook::Inst()->ExecuteCommandLists)(NumCommandLists, ppCommandLists);
}

DX12_Hook::DX12_Hook():
    initialized(false),
    pCmdQueue(nullptr),
    bufferCount(0),
    pCmdAlloc(nullptr),
    pSrvDescHeap(nullptr),
    pCmdList(nullptr),
    pRtvDescHeap(nullptr),
    hooked(false),
    Present(nullptr),
    ResizeBuffers(nullptr),
    ResizeTarget(nullptr),
    ExecuteCommandLists(nullptr)
{
    _library = LoadLibrary(DX12_DLL);

    PRINT_DEBUG("DX12 support is experimental, don't complain if it doesn't work as expected.\n");
}

DX12_Hook::~DX12_Hook()
{
    PRINT_DEBUG("DX12 Hook removed\n");

    if (initialized)
    {
        pSrvDescHeap->Release();
        for (UINT i = 0; i < bufferCount; ++i)
        {
            pCmdAlloc[i]->Release();
            pBackBuffer[i]->Release();
        }
        pRtvDescHeap->Release();
        delete[]pCmdAlloc;
        delete[]pBackBuffer;

        ImGui_ImplDX12_InvalidateDeviceObjects();
        ImGui::DestroyContext();

        initialized = false;
    }

    FreeLibrary(reinterpret_cast<HMODULE>(_library));

    _inst = nullptr;
}

DX12_Hook* DX12_Hook::Inst()
{
    if (_inst == nullptr)
        _inst = new DX12_Hook();

    return _inst;
}

const char* DX12_Hook::get_lib_name() const
{
    return DX12_DLL;
}

void DX12_Hook::loadFunctions(ID3D12CommandQueue* pCommandQueue, IDXGISwapChain *pSwapChain)
{
    void** vTable;
    
    vTable = *reinterpret_cast<void***>(pCommandQueue);
#define LOAD_FUNC(X) (void*&)X = vTable[(int)ID3D12CommandQueueVTable::X]
    LOAD_FUNC(ExecuteCommandLists);
#undef LOAD_FUNC

    vTable = *reinterpret_cast<void***>(pSwapChain);
#define LOAD_FUNC(X) (void*&)X = vTable[(int)IDXGISwapChainVTable::X]
    LOAD_FUNC(Present);
    LOAD_FUNC(ResizeBuffers);
    LOAD_FUNC(ResizeTarget);
#undef LOAD_FUNC
}

#endif//EMU_OVERLAY