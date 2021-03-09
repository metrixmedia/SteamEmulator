#include "DX11_Hook.h"
#include "Windows_Hook.h"
#include "../Renderer_Detector.h"
#include "../../dll/dll.h"

#ifdef EMU_OVERLAY

#include <imgui.h>
#include <impls/windows/imgui_impl_dx11.h>

DX11_Hook* DX11_Hook::_inst = nullptr;

HRESULT GetDeviceAndCtxFromSwapchain(IDXGISwapChain* pSwapChain, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext)
{
    HRESULT ret = pSwapChain->GetDevice(IID_PPV_ARGS(ppDevice));

    if (SUCCEEDED(ret))
        (*ppDevice)->GetImmediateContext(ppContext);

    return ret;
}

bool DX11_Hook::start_hook()
{
    bool res = true;
    if (!hooked)
    {
        if (!Windows_Hook::Inst()->start_hook())
            return false;

        PRINT_DEBUG("Hooked DirectX 11\n");
        hooked = true;

        Renderer_Detector::Inst().renderer_found(this);

        BeginHook();
        HookFuncs(
            std::make_pair<void**, void*>(&(PVOID&)DX11_Hook::Present, &DX11_Hook::MyPresent),
            std::make_pair<void**, void*>(&(PVOID&)DX11_Hook::ResizeTarget, &DX11_Hook::MyResizeTarget),
            std::make_pair<void**, void*>(&(PVOID&)DX11_Hook::ResizeBuffers, &DX11_Hook::MyResizeBuffers)
        );
        EndHook();

        get_steam_client()->steam_overlay->HookReady();
    }
    return res;
}

void DX11_Hook::resetRenderState()
{
    if (initialized)
    {
        if (mainRenderTargetView) {
            mainRenderTargetView->Release();
            mainRenderTargetView = NULL;
        }

        pContext->Release();

        ImGui_ImplDX11_Shutdown();
        Windows_Hook::Inst()->resetRenderState();
        ImGui::DestroyContext();

        initialized = false;
    }
}

// Try to make this function and overlay's proc as short as possible or it might affect game's fps.
void DX11_Hook::prepareForOverlay(IDXGISwapChain* pSwapChain)
{
    DXGI_SWAP_CHAIN_DESC desc;
    pSwapChain->GetDesc(&desc);

    if (!initialized)
    {
        ID3D11Device* pDevice = nullptr;
        if (FAILED(GetDeviceAndCtxFromSwapchain(pSwapChain, &pDevice, &pContext)))
            return;

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL;

        ID3D11Texture2D* pBackBuffer;
        pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

        ID3D11RenderTargetView *get_targets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        pContext->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, get_targets, NULL);
        bool bind_target = true;

        for (unsigned i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
            if (get_targets[i]) {
                ID3D11Resource *res = NULL;
                get_targets[i]->GetResource(&res);
                if (res) {
                    if (res == (ID3D11Resource*)pBackBuffer) {
                        bind_target = false;
                    }

                    res->Release();
                }

                get_targets[i]->Release();
            } else {
                break;
            }
        }

        if (bind_target) {
            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
        }

        pBackBuffer->Release();

        ImGui_ImplDX11_Init(pDevice, pContext);

        pDevice->Release();

        get_steam_client()->steam_overlay->CreateFonts();

        initialized = true;
    }

    if (ImGui_ImplDX11_NewFrame())
    {
        Windows_Hook::Inst()->prepareForOverlay(desc.OutputWindow);

        ImGui::NewFrame();

        get_steam_client()->steam_overlay->OverlayProc();

        ImGui::Render();

        if (mainRenderTargetView) {
            pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
        }

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
}

HRESULT STDMETHODCALLTYPE DX11_Hook::MyPresent(IDXGISwapChain *_this, UINT SyncInterval, UINT Flags)
{
    DX11_Hook::Inst()->prepareForOverlay(_this);

    return (_this->*DX11_Hook::Inst()->Present)(SyncInterval, Flags);
}

HRESULT STDMETHODCALLTYPE DX11_Hook::MyResizeTarget(IDXGISwapChain* _this, const DXGI_MODE_DESC* pNewTargetParameters)
{
    DX11_Hook::Inst()->resetRenderState();
    return (_this->*DX11_Hook::Inst()->ResizeTarget)(pNewTargetParameters);
}

HRESULT STDMETHODCALLTYPE DX11_Hook::MyResizeBuffers(IDXGISwapChain* _this, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    DX11_Hook::Inst()->resetRenderState();
    return (_this->*DX11_Hook::Inst()->ResizeBuffers)(BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

DX11_Hook::DX11_Hook():
    initialized(false),
    hooked(false),
    pContext(nullptr),
    mainRenderTargetView(nullptr),
    Present(nullptr),
    ResizeBuffers(nullptr),
    ResizeTarget(nullptr)
{
    _library = LoadLibrary(DX11_DLL);
}

DX11_Hook::~DX11_Hook()
{
    PRINT_DEBUG("DX11 Hook removed\n");

    if (initialized)
    {
        if (mainRenderTargetView) {
            mainRenderTargetView->Release();
            mainRenderTargetView = NULL;
        }

        pContext->Release();

        ImGui_ImplDX11_InvalidateDeviceObjects();
        ImGui::DestroyContext();

        initialized = false;
    }

    FreeLibrary(reinterpret_cast<HMODULE>(_library));

    _inst = nullptr;
}

DX11_Hook* DX11_Hook::Inst()
{
    if (_inst == nullptr)
        _inst = new DX11_Hook;

    return _inst;
}

const char* DX11_Hook::get_lib_name() const
{
    return DX11_DLL;
}

void DX11_Hook::loadFunctions(IDXGISwapChain *pSwapChain)
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