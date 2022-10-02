/*
 * Copyright (C) Nemirtingas
 * This file is part of the ingame overlay project
 *
 * The ingame overlay project is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * The ingame overlay project is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the ingame overlay project; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "DX11_Hook.h"
#include "Windows_Hook.h"

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>

DX11_Hook* DX11_Hook::_inst = nullptr;

template<typename T>
inline void SafeRelease(T*& pUnk)
{
    if (pUnk != nullptr)
    {
        pUnk->Release();
        pUnk = nullptr;
    }
}

static HRESULT GetDeviceAndCtxFromSwapchain(IDXGISwapChain* pSwapChain, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext)
{
    HRESULT ret = pSwapChain->GetDevice(IID_PPV_ARGS(ppDevice));

    if (SUCCEEDED(ret))
        (*ppDevice)->GetImmediateContext(ppContext);

    return ret;
}

bool DX11_Hook::StartHook(std::function<bool(bool)> key_combination_callback, std::set<ingame_overlay::ToggleKey> toggle_keys)
{
    if (!_Hooked)
    {
        if (Present == nullptr || ResizeTarget == nullptr || ResizeBuffers == nullptr)
        {
            SPDLOG_WARN("Failed to hook DirectX 11: Rendering functions missing.");
            return false;
        }

        if (!Windows_Hook::Inst()->StartHook(key_combination_callback, toggle_keys))
            return false;

        _WindowsHooked = true;

        SPDLOG_INFO("Hooked DirectX 11");
        _Hooked = true;

        BeginHook();
        HookFuncs(
            std::make_pair<void**, void*>(&(PVOID&)Present      , &DX11_Hook::MyPresent),
            std::make_pair<void**, void*>(&(PVOID&)ResizeTarget , &DX11_Hook::MyResizeTarget),
            std::make_pair<void**, void*>(&(PVOID&)ResizeBuffers, &DX11_Hook::MyResizeBuffers)
        );
        if (Present1 != nullptr)
        {
            HookFuncs(
                std::make_pair<void**, void*>(&(PVOID&)Present1, &DX11_Hook::MyPresent1)
            );
        }
        EndHook();
    }
    return true;
}

bool DX11_Hook::IsStarted()
{
    return _Hooked;
}

void DX11_Hook::_ResetRenderState()
{
    if (_Initialized)
    {
        OverlayHookReady(false);

        ImGui_ImplDX11_Shutdown();
        Windows_Hook::Inst()->ResetRenderState();
        //ImGui::DestroyContext();

        SafeRelease(mainRenderTargetView);
        SafeRelease(pContext);
        SafeRelease(pDevice);

        _Initialized = false;
    }
}

// Try to make this function and overlay's proc as short as possible or it might affect game's fps.
void DX11_Hook::_PrepareForOverlay(IDXGISwapChain* pSwapChain)
{
    DXGI_SWAP_CHAIN_DESC desc;
    pSwapChain->GetDesc(&desc);

    if (!_Initialized)
    {
        pDevice = nullptr;
        if (FAILED(GetDeviceAndCtxFromSwapchain(pSwapChain, &pDevice, &pContext)))
            return;

        ID3D11Texture2D* pBackBuffer;
        pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

        pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);

        //ID3D11RenderTargetView* targets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        //pContext->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, targets, NULL);
        //bool bind_target = true;
        //
        //for (unsigned i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT && targets[i] != nullptr; ++i)
        //{
        //    ID3D11Resource* res = NULL;
        //    targets[i]->GetResource(&res);
        //    if (res)
        //    {
        //        if (res == (ID3D11Resource*)pBackBuffer)
        //        {
        //            pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
        //        }
        //
        //        res->Release();
        //    }
        //
        //    targets[i]->Release();
        //}
        
        SafeRelease(pBackBuffer);
        
        if (mainRenderTargetView == nullptr)
            return;
        
        if(ImGui::GetCurrentContext() == nullptr)
            ImGui::CreateContext((ImFontAtlas *)ImGuiFontAtlas);
        
        ImGui_ImplDX11_Init(pDevice, pContext);
        
        Windows_Hook::Inst()->SetInitialWindowSize(desc.OutputWindow);

        _Initialized = true;
        OverlayHookReady(true);
    }

    if (ImGui_ImplDX11_NewFrame() && Windows_Hook::Inst()->PrepareForOverlay(desc.OutputWindow))
    {
        ImGui::NewFrame();
    
        OverlayProc();
    
        ImGui::Render();

        if (mainRenderTargetView)
        {
            pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
        }
    
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
}

HRESULT STDMETHODCALLTYPE DX11_Hook::MyPresent(IDXGISwapChain *_this, UINT SyncInterval, UINT Flags)
{
    auto inst = DX11_Hook::Inst();
    inst->_PrepareForOverlay(_this);
    return (_this->*inst->Present)(SyncInterval, Flags);
}

HRESULT STDMETHODCALLTYPE DX11_Hook::MyResizeTarget(IDXGISwapChain* _this, const DXGI_MODE_DESC* pNewTargetParameters)
{
    auto inst = DX11_Hook::Inst();
    inst->_ResetRenderState();
    return (_this->*inst->ResizeTarget)(pNewTargetParameters);
}

HRESULT STDMETHODCALLTYPE DX11_Hook::MyResizeBuffers(IDXGISwapChain* _this, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    auto inst = DX11_Hook::Inst();
    inst->_ResetRenderState();
    return (_this->*inst->ResizeBuffers)(BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

HRESULT STDMETHODCALLTYPE DX11_Hook::MyPresent1(IDXGISwapChain1* _this, UINT SyncInterval, UINT Flags, const DXGI_PRESENT_PARAMETERS* pPresentParameters)
{
    auto inst = DX11_Hook::Inst();
    inst->_PrepareForOverlay(_this);
    return (_this->*inst->Present1)(SyncInterval, Flags, pPresentParameters);
}

DX11_Hook::DX11_Hook():
    _Initialized(false),
    _Hooked(false),
    _WindowsHooked(false),
    pContext(nullptr),
    mainRenderTargetView(nullptr),
    Present(nullptr),
    ResizeBuffers(nullptr),
    ResizeTarget(nullptr),
    Present1(nullptr)
{
}

DX11_Hook::~DX11_Hook()
{
    SPDLOG_INFO("DX11 Hook removed");

    if (_WindowsHooked)
        delete Windows_Hook::Inst();

    if (_Initialized)
    {
        SafeRelease(mainRenderTargetView);
        SafeRelease(pContext);

        ImGui_ImplDX11_InvalidateDeviceObjects();
        ImGui::DestroyContext();

        _Initialized = false;
    }

    _inst = nullptr;
}

DX11_Hook* DX11_Hook::Inst()
{
    if (_inst == nullptr)
        _inst = new DX11_Hook;

    return _inst;
}

std::string DX11_Hook::GetLibraryName() const
{
    return LibraryName;
}

void DX11_Hook::LoadFunctions(
    decltype(Present) PresentFcn,
    decltype(ResizeBuffers) ResizeBuffersFcn,
    decltype(ResizeTarget) ResizeTargetFcn,
    decltype(Present1) Present1Fcn)
{
    Present = PresentFcn;
    ResizeBuffers = ResizeBuffersFcn;
    ResizeTarget = ResizeTargetFcn;
    Present1 = Present1Fcn;
}

std::weak_ptr<uint64_t> DX11_Hook::CreateImageResource(const void* image_data, uint32_t width, uint32_t height)
{
    ID3D11ShaderResourceView** resource = new ID3D11ShaderResourceView*(nullptr);

    // Create texture
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = nullptr;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    pDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    if (pTexture != nullptr)
    {
        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;

        pDevice->CreateShaderResourceView(pTexture, &srvDesc, resource);
        // Release Texture, the shader resource increases the reference count.
        pTexture->Release();
    }

    if (*resource == nullptr)
        return std::shared_ptr<uint64_t>();

    auto ptr = std::shared_ptr<uint64_t>((uint64_t*)resource, [](uint64_t* handle)
    {
        if(handle != nullptr)
        {
            ID3D11ShaderResourceView** resource = reinterpret_cast<ID3D11ShaderResourceView**>(handle);
            (*resource)->Release();
            delete resource;
        }
    });

    _ImageResources.emplace(ptr);

    return ptr;
}

void DX11_Hook::ReleaseImageResource(std::weak_ptr<uint64_t> resource)
{
    auto ptr = resource.lock();
    if (ptr)
    {
        auto it = _ImageResources.find(ptr);
        if (it != _ImageResources.end())
            _ImageResources.erase(it);
    }
}