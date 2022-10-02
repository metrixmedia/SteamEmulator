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

#include "DX10_Hook.h"
#include "Windows_Hook.h"

#include <imgui.h>
#include <backends/imgui_impl_dx10.h>

DX10_Hook* DX10_Hook::_inst = nullptr;

template<typename T>
inline void SafeRelease(T*& pUnk)
{
    if (pUnk != nullptr)
    {
        pUnk->Release();
        pUnk = nullptr;
    }
}

bool DX10_Hook::StartHook(std::function<bool(bool)> key_combination_callback, std::set<ingame_overlay::ToggleKey> toggle_keys)
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

        SPDLOG_INFO("Hooked DirectX 10");
        _Hooked = true;

        BeginHook();
        HookFuncs(
            std::make_pair<void**, void*>(&(PVOID&)Present      , &DX10_Hook::MyPresent),
            std::make_pair<void**, void*>(&(PVOID&)ResizeTarget , &DX10_Hook::MyResizeTarget),
            std::make_pair<void**, void*>(&(PVOID&)ResizeBuffers, &DX10_Hook::MyResizeBuffers)
        );
        if (Present1 != nullptr)
        {
            HookFuncs(
                std::make_pair<void**, void*>(&(PVOID&)Present1, &DX10_Hook::MyPresent1)
            );
        }
        EndHook();
    }
    return true;
}

bool DX10_Hook::IsStarted()
{
    return _Hooked;
}

void DX10_Hook::_ResetRenderState()
{
    if (_Initialized)
    {
        OverlayHookReady(false);

        ImGui_ImplDX10_Shutdown();
        Windows_Hook::Inst()->ResetRenderState();
        ImGui::DestroyContext();

        SafeRelease(mainRenderTargetView);
        SafeRelease(pDevice);

        _Initialized = false;
    }
}

// Try to make this function and overlay's proc as short as possible or it might affect game's fps.
void DX10_Hook::_PrepareForOverlay(IDXGISwapChain* pSwapChain)
{
    DXGI_SWAP_CHAIN_DESC desc;
    pSwapChain->GetDesc(&desc);

    if (!_Initialized)
    {
        if (FAILED(pSwapChain->GetDevice(IID_PPV_ARGS(&pDevice))))
            return;

        ID3D10Texture2D* pBackBuffer = nullptr;

        pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        if (pBackBuffer == nullptr)
        {
            pDevice->Release();
            return;
        }

        pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &mainRenderTargetView);
        pBackBuffer->Release();

        ImGui::CreateContext((ImFontAtlas *)ImGuiFontAtlas);
        ImGui_ImplDX10_Init(pDevice);

        Windows_Hook::Inst()->SetInitialWindowSize(desc.OutputWindow);

        _Initialized = true;
        OverlayHookReady(true);
    }

    if (ImGui_ImplDX10_NewFrame() && Windows_Hook::Inst()->PrepareForOverlay(desc.OutputWindow))
    {
        ImGui::NewFrame();

        OverlayProc();

        ImGui::Render();

        pDevice->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
        ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());
    }
}

HRESULT STDMETHODCALLTYPE DX10_Hook::MyPresent(IDXGISwapChain *_this, UINT SyncInterval, UINT Flags)
{
    auto inst= DX10_Hook::Inst();
    inst->_PrepareForOverlay(_this);
    return (_this->*inst->Present)(SyncInterval, Flags);
}

HRESULT STDMETHODCALLTYPE DX10_Hook::MyResizeTarget(IDXGISwapChain* _this, const DXGI_MODE_DESC* pNewTargetParameters)
{
    auto inst= DX10_Hook::Inst();
    inst->_ResetRenderState();
    return (_this->*inst->ResizeTarget)(pNewTargetParameters);
}

HRESULT STDMETHODCALLTYPE DX10_Hook::MyResizeBuffers(IDXGISwapChain* _this, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    auto inst= DX10_Hook::Inst();
    inst->_ResetRenderState();
    return (_this->*inst->ResizeBuffers)(BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

HRESULT STDMETHODCALLTYPE DX10_Hook::MyPresent1(IDXGISwapChain1* _this, UINT SyncInterval, UINT Flags, const DXGI_PRESENT_PARAMETERS* pPresentParameters)
{
    auto inst = DX10_Hook::Inst();
    inst->_PrepareForOverlay(_this);
    return (_this->*inst->Present1)(SyncInterval, Flags, pPresentParameters);
}

DX10_Hook::DX10_Hook():
    _Initialized(false),
    _Hooked(false),
    _WindowsHooked(false),
    pDevice(nullptr),
    mainRenderTargetView(nullptr),
    Present(nullptr),
    ResizeBuffers(nullptr),
    ResizeTarget(nullptr),
    Present1(nullptr)
{
}

DX10_Hook::~DX10_Hook()
{
    //SPDLOG_INFO("DX10 Hook removed");

    if (_WindowsHooked)
        delete Windows_Hook::Inst();

    if (_Initialized)
    {
        mainRenderTargetView->Release();

        ImGui_ImplDX10_InvalidateDeviceObjects();
        ImGui::DestroyContext();

        _Initialized = false;
    }

    _inst = nullptr;
}

DX10_Hook* DX10_Hook::Inst()
{
    if (_inst == nullptr)   
        _inst = new DX10_Hook;

    return _inst;
}

std::string DX10_Hook::GetLibraryName() const
{
    return LibraryName;
}

void DX10_Hook::LoadFunctions(
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

std::weak_ptr<uint64_t> DX10_Hook::CreateImageResource(const void* image_data, uint32_t width, uint32_t height)
{
    ID3D10ShaderResourceView** resource = new ID3D10ShaderResourceView*(nullptr);

    // Create texture
    D3D10_TEXTURE2D_DESC desc = {};
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D10_USAGE_DEFAULT;
    desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D10Texture2D* pTexture = nullptr;
    D3D10_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    pDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    if (pTexture != nullptr)
    {
        // Create texture view
        D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;

        pDevice->CreateShaderResourceView(pTexture, &srvDesc, resource);
        // Release Texure, the shader resource increases the reference count.
        pTexture->Release();
    }

    if (*resource == nullptr)
        return std::shared_ptr<uint64_t>();

    auto ptr = std::shared_ptr<uint64_t>((uint64_t*)resource, [](uint64_t* handle)
    {
        if (handle != nullptr)
        {
            ID3D10ShaderResourceView** resource = reinterpret_cast<ID3D10ShaderResourceView**>(handle);
            (*resource)->Release();
            delete resource;
        }
    });

    _ImageResources.emplace(ptr);

    return ptr;
}

void DX10_Hook::ReleaseImageResource(std::weak_ptr<uint64_t> resource)
{
    auto ptr = resource.lock();
    if (ptr)
    {
        auto it = _ImageResources.find(ptr);
        if (it != _ImageResources.end())
            _ImageResources.erase(it);
    }
}