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

#include "DX9_Hook.h"
#include "Windows_Hook.h"
#include "DirectX_VTables.h"

#include <imgui.h>
#include <backends/imgui_impl_dx9.h>

DX9_Hook* DX9_Hook::_inst = nullptr;

template<typename T>
inline void SafeRelease(T*& pUnk)
{
    if (pUnk != nullptr)
    {
        pUnk->Release();
        pUnk = nullptr;
    }
}

bool DX9_Hook::StartHook(std::function<bool(bool)> key_combination_callback, std::set<ingame_overlay::ToggleKey> toggle_keys)
{
    if (!_Hooked)
    {
        if (Reset == nullptr || Present == nullptr)
        {
            SPDLOG_WARN("Failed to hook DirectX 9: Rendering functions missing.");
            return false;
        }

        if (!Windows_Hook::Inst()->StartHook(key_combination_callback, toggle_keys))
            return false;

        _WindowsHooked = true;

        SPDLOG_INFO("Hooked DirectX 9");
        _Hooked = true;

        BeginHook();
        HookFuncs(
            std::make_pair<void**, void*>(&(PVOID&)Reset, &DX9_Hook::MyReset),
            std::make_pair<void**, void*>(&(PVOID&)Present, &DX9_Hook::MyPresent)
        );
        if (PresentEx != nullptr)
        {
            HookFuncs(
                std::make_pair<void**, void*>(&(PVOID&)PresentEx, &DX9_Hook::MyPresentEx)
            );
        }
        if (SwapChainPresent != nullptr)
        {
            HookFuncs(
                std::make_pair<void**, void*>(&(PVOID&)SwapChainPresent, &DX9_Hook::MySwapChainPresent)
            );
        }
        EndHook();
    }
    return true;
}

bool DX9_Hook::IsStarted()
{
    return _Hooked;
}

void DX9_Hook::_ResetRenderState()
{
    if (_Initialized)
    {
        OverlayHookReady(false);

        ImGui_ImplDX9_Shutdown();
        Windows_Hook::Inst()->ResetRenderState();
        ImGui::DestroyContext();

        SafeRelease(_pDevice);
        
        _LastWindow = nullptr;
        _Initialized = false;
    }
}

// Try to make this function and overlay's proc as short as possible or it might affect game's fps.
void DX9_Hook::_PrepareForOverlay(IDirect3DDevice9 *pDevice, HWND destWindow)
{
    if (!destWindow)
    {
        IDirect3DSwapChain9 *pSwapChain = nullptr;
        if (pDevice->GetSwapChain(0, &pSwapChain) == D3D_OK)
        {
            D3DPRESENT_PARAMETERS params;
            if (pSwapChain->GetPresentParameters(&params) == D3D_OK)
            {
                destWindow = params.hDeviceWindow;
            }

            pSwapChain->Release();
        }
    }

    //Is this necessary anymore?
    if (!destWindow)
    {
        D3DDEVICE_CREATION_PARAMETERS param;
        pDevice->GetCreationParameters(&param);
        destWindow = param.hFocusWindow;
    }

    // Workaround to detect if we changed window.
    if (destWindow != _LastWindow || _pDevice != pDevice)
        _ResetRenderState();

    if (!_Initialized)
    {
        pDevice->AddRef();
        _pDevice = pDevice;

        ImGui::CreateContext((ImFontAtlas *)ImGuiFontAtlas);
        ImGui_ImplDX9_Init(pDevice);

        _LastWindow = destWindow;

        Windows_Hook::Inst()->SetInitialWindowSize(destWindow);

        _Initialized = true;
        OverlayHookReady(true);
    }

    if (ImGui_ImplDX9_NewFrame() && Windows_Hook::Inst()->PrepareForOverlay(destWindow))
    {
        ImGui::NewFrame();

        OverlayProc();

        ImGui::Render();

        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }
}

HRESULT STDMETHODCALLTYPE DX9_Hook::MyReset(IDirect3DDevice9* _this, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    auto inst = DX9_Hook::Inst();
    inst->_ResetRenderState();
    return (_this->*inst->Reset)(pPresentationParameters);
}

HRESULT STDMETHODCALLTYPE DX9_Hook::MyPresent(IDirect3DDevice9* _this, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
    auto inst = DX9_Hook::Inst();
    inst->_PrepareForOverlay(_this, hDestWindowOverride);
    return (_this->*inst->Present)(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

HRESULT STDMETHODCALLTYPE DX9_Hook::MyPresentEx(IDirect3DDevice9Ex* _this, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags)
{
    auto inst = DX9_Hook::Inst();
    inst->_PrepareForOverlay(_this, hDestWindowOverride);
    return (_this->*inst->PresentEx)(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
}

HRESULT STDMETHODCALLTYPE DX9_Hook::MySwapChainPresent(IDirect3DSwapChain9* _this, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags)
{
    IDirect3DDevice9* pDevice;
    auto inst = DX9_Hook::Inst();

    if (SUCCEEDED(_this->GetDevice(&pDevice)))
    {
        HWND destWindow = hDestWindowOverride;
        if (!destWindow)
        {
            D3DPRESENT_PARAMETERS param;
            if (_this->GetPresentParameters(&param) == D3D_OK)
            {
                destWindow = param.hDeviceWindow;
            }
        }

        inst->_PrepareForOverlay(pDevice, destWindow);
        pDevice->Release();
    }
    return (_this->*inst->SwapChainPresent)(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
}

DX9_Hook::DX9_Hook():
    _Initialized(false),
    _Hooked(false),
    _WindowsHooked(false),
    _LastWindow(nullptr),
    Present(nullptr),
    PresentEx(nullptr),
    Reset(nullptr)
{
}

DX9_Hook::~DX9_Hook()
{
    SPDLOG_INFO("DX9 Hook removed");

    if (_WindowsHooked)
        delete Windows_Hook::Inst();

    if (_Initialized)
    {
        ImGui_ImplDX9_InvalidateDeviceObjects();
        ImGui::DestroyContext();
    }

    _inst = nullptr;
}

DX9_Hook* DX9_Hook::Inst()
{
    if( _inst == nullptr )
        _inst = new DX9_Hook;

    return _inst;
}

std::string DX9_Hook::GetLibraryName() const
{
    return LibraryName;
}

void DX9_Hook::LoadFunctions(decltype(Present) PresentFcn, decltype(Reset) ResetFcn, decltype(PresentEx) PresentExFcn, decltype(&IDirect3DSwapChain9::Present) SwapChainPresentFcn)
{
    Present = PresentFcn;
    Reset = ResetFcn;

    PresentEx = PresentExFcn;

    SwapChainPresent = SwapChainPresentFcn;
}

std::weak_ptr<uint64_t> DX9_Hook::CreateImageResource(const void* image_data, uint32_t width, uint32_t height)
{
    IDirect3DTexture9** pTexture = new IDirect3DTexture9*(nullptr);

    _pDevice->CreateTexture(
        width,
        height,
        1,
        D3DUSAGE_DYNAMIC,
        D3DFMT_A8R8G8B8,
        D3DPOOL_DEFAULT,
        pTexture,
        nullptr
    );

    if (*pTexture != nullptr)
    {
        D3DLOCKED_RECT rect;
        if (SUCCEEDED((*pTexture)->LockRect(0, &rect, nullptr, D3DLOCK_DISCARD)))
        {
            const uint32_t* pixels = reinterpret_cast<const uint32_t*>(image_data);
            uint8_t* texture_bits = reinterpret_cast<uint8_t*>(rect.pBits);
            for (int32_t i = 0; i < height; ++i)
            {
                for (int32_t j = 0; j < width; ++j)
                {
                    // RGBA to ARGB Conversion, DX9 doesn't have a RGBA loader
                    uint32_t color = *pixels++;
                    reinterpret_cast<uint32_t*>(texture_bits)[j] = ((color & 0xff) << 16) | (color & 0xff00) | ((color & 0xff0000) >> 16) | (color & 0xff000000);
                }
                texture_bits += rect.Pitch;
            }

            if (FAILED((*pTexture)->UnlockRect(0)))
            {
                (*pTexture)->Release();
                delete pTexture;
                pTexture = nullptr;
            }
        }
        else
        {
            (*pTexture)->Release();
            delete pTexture;
            pTexture = nullptr;
        }
    }

    if (pTexture == nullptr)
        return std::shared_ptr<uint64_t>();

    auto ptr = std::shared_ptr<uint64_t>((uint64_t*)pTexture, [](uint64_t* handle)
    {
        if (handle != nullptr)
        {
            IDirect3DTexture9** resource = reinterpret_cast<IDirect3DTexture9**>(handle);
            (*resource)->Release();
            delete resource;
        }
    });

    _ImageResources.emplace(ptr);

    return ptr;
}

void DX9_Hook::ReleaseImageResource(std::weak_ptr<uint64_t> resource)
{
    auto ptr = resource.lock();
    if (ptr)
    {
        auto it = _ImageResources.find(ptr);
        if (it != _ImageResources.end())
            _ImageResources.erase(it);
    }
}