#include "DX10_Hook.h"
#include "Windows_Hook.h"
#include "../Renderer_Detector.h"
#include "../../dll/dll.h"

#ifdef EMU_OVERLAY

#include <imgui.h>
#include <impls/windows/imgui_impl_dx10.h>

DX10_Hook* DX10_Hook::_inst = nullptr;

bool DX10_Hook::start_hook()
{
    bool res = true;
    if (!hooked)
    {
        if (!Windows_Hook::Inst()->start_hook())
            return false;

        PRINT_DEBUG("Hooked DirectX 10\n");
        hooked = true;

        Renderer_Detector::Inst().renderer_found(this);

        BeginHook();
        HookFuncs(
            std::make_pair<void**, void*>(&(PVOID&)DX10_Hook::Present, &DX10_Hook::MyPresent),
            std::make_pair<void**, void*>(&(PVOID&)DX10_Hook::ResizeTarget, &DX10_Hook::MyResizeTarget),
            std::make_pair<void**, void*>(&(PVOID&)DX10_Hook::ResizeBuffers, &DX10_Hook::MyResizeBuffers)
        );
        EndHook();

        get_steam_client()->steam_overlay->HookReady();
    }
    return res;
}

void DX10_Hook::resetRenderState()
{
    if (initialized)
    {
        mainRenderTargetView->Release();

        ImGui_ImplDX10_Shutdown();
        Windows_Hook::Inst()->resetRenderState();
        ImGui::DestroyContext();

        initialized = false;
    }
}

// Try to make this function and overlay's proc as short as possible or it might affect game's fps.
void DX10_Hook::prepareForOverlay(IDXGISwapChain* pSwapChain)
{
    DXGI_SWAP_CHAIN_DESC desc;
    pSwapChain->GetDesc(&desc);

    if (!initialized)
    {
        if (FAILED(pSwapChain->GetDevice(IID_PPV_ARGS(&pDevice))))
            return;

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL;

        ID3D10Texture2D* pBackBuffer;

        pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
        pBackBuffer->Release();

        ImGui_ImplDX10_Init(pDevice);

        pDevice->Release();

        get_steam_client()->steam_overlay->CreateFonts();

        initialized = true;
    }

    if (ImGui_ImplDX10_NewFrame())
    {
        Windows_Hook::Inst()->prepareForOverlay(desc.OutputWindow);

        ImGui::NewFrame();

        get_steam_client()->steam_overlay->OverlayProc();

        ImGui::Render();

        pDevice->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
        ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());
    }
}

HRESULT STDMETHODCALLTYPE DX10_Hook::MyPresent(IDXGISwapChain *_this, UINT SyncInterval, UINT Flags)
{
    DX10_Hook::Inst()->prepareForOverlay(_this);
    return (_this->*DX10_Hook::Inst()->Present)(SyncInterval, Flags);
}

HRESULT STDMETHODCALLTYPE DX10_Hook::MyResizeTarget(IDXGISwapChain* _this, const DXGI_MODE_DESC* pNewTargetParameters)
{
    DX10_Hook::Inst()->resetRenderState();
    return (_this->*DX10_Hook::Inst()->ResizeTarget)(pNewTargetParameters);
}

HRESULT STDMETHODCALLTYPE DX10_Hook::MyResizeBuffers(IDXGISwapChain* _this, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    DX10_Hook::Inst()->resetRenderState();
    return (_this->*DX10_Hook::Inst()->ResizeBuffers)(BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

DX10_Hook::DX10_Hook():
    initialized(false),
    hooked(false),
    pDevice(nullptr),
    mainRenderTargetView(nullptr),
    Present(nullptr),
    ResizeBuffers(nullptr),
    ResizeTarget(nullptr)
{
    _library = LoadLibrary(DX10_DLL);
}

DX10_Hook::~DX10_Hook()
{
    PRINT_DEBUG("DX10 Hook removed\n");

    if (initialized)
    {
        mainRenderTargetView->Release();

        ImGui_ImplDX10_InvalidateDeviceObjects();
        ImGui::DestroyContext();

        initialized = false;
    }

    FreeLibrary(reinterpret_cast<HMODULE>(_library));

    _inst = nullptr;
}

DX10_Hook* DX10_Hook::Inst()
{
    if (_inst == nullptr)   
        _inst = new DX10_Hook;

    return _inst;
}

const char* DX10_Hook::get_lib_name() const
{
    return DX10_DLL;
}

void DX10_Hook::loadFunctions(IDXGISwapChain *pSwapChain)
{
    void** vTable;

    vTable = *reinterpret_cast<void***>(pSwapChain);
#define LOAD_FUNC(X) (void*&)X = vTable[(int)IDXGISwapChainVTable::X]
    LOAD_FUNC(Present);
    LOAD_FUNC(ResizeBuffers);
    LOAD_FUNC(ResizeTarget);
#undef LOAD_FUNC
}

#endif//EMU_OVERLAY