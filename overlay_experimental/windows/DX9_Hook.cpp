#include "DX9_Hook.h"
#include "Windows_Hook.h"
#include "../Renderer_Detector.h"
#include "../../dll/dll.h"

#ifdef EMU_OVERLAY

#include <imgui.h>
#include <impls/windows/imgui_impl_dx9.h>

DX9_Hook* DX9_Hook::_inst = nullptr;

bool DX9_Hook::start_hook()
{
    if (!hooked)
    {
        if (!Windows_Hook::Inst()->start_hook())
            return false;

        PRINT_DEBUG("Hooked DirectX 9\n");
        hooked = true;
        
        Renderer_Detector::Inst().renderer_found(this);

        BeginHook();
        HookFuncs(
            std::make_pair<void**, void*>(&(PVOID&)Reset, &DX9_Hook::MyReset),
            std::make_pair<void**, void*>(&(PVOID&)Present, &DX9_Hook::MyPresent)
        );
        if (PresentEx != nullptr)
        {
            HookFuncs(
                std::make_pair<void**, void*>(&(PVOID&)PresentEx, &DX9_Hook::MyPresentEx)
                //std::make_pair<void**, void*>(&(PVOID&)EndScene, &DX9_Hook::MyEndScene)
            );
        }
        EndHook();

        get_steam_client()->steam_overlay->HookReady();
    }
    return true;
}

void DX9_Hook::resetRenderState()
{
    if (initialized)
    {
        initialized = false;
        ImGui_ImplDX9_Shutdown();
        Windows_Hook::Inst()->resetRenderState();
        ImGui::DestroyContext();
    }
}

// Try to make this function and overlay's proc as short as possible or it might affect game's fps.
void DX9_Hook::prepareForOverlay(IDirect3DDevice9 *pDevice)
{
    D3DDEVICE_CREATION_PARAMETERS param;
    pDevice->GetCreationParameters(&param);

    // Workaround to detect if we changed window.
    if (param.hFocusWindow != Windows_Hook::Inst()->GetGameHwnd())
        resetRenderState();

    if (!initialized)
    {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL;

        ImGui_ImplDX9_Init(pDevice);

        get_steam_client()->steam_overlay->CreateFonts();

        initialized = true;
    }
    
    if (ImGui_ImplDX9_NewFrame())
    {
        Windows_Hook::Inst()->prepareForOverlay(param.hFocusWindow);

        ImGui::NewFrame();

        get_steam_client()->steam_overlay->OverlayProc();

        ImGui::Render();

        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }
}

HRESULT STDMETHODCALLTYPE DX9_Hook::MyReset(IDirect3DDevice9* _this, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    DX9_Hook::Inst()->resetRenderState();
    return (_this->*DX9_Hook::Inst()->Reset)(pPresentationParameters);
}

HRESULT STDMETHODCALLTYPE DX9_Hook::MyEndScene(IDirect3DDevice9* _this)
{   
    if( !DX9_Hook::Inst()->uses_present )
        DX9_Hook::Inst()->prepareForOverlay(_this);
    return (_this->*DX9_Hook::Inst()->EndScene)();
}

HRESULT STDMETHODCALLTYPE DX9_Hook::MyPresent(IDirect3DDevice9* _this, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
    DX9_Hook::Inst()->uses_present = true;
    DX9_Hook::Inst()->prepareForOverlay(_this);
    return (_this->*DX9_Hook::Inst()->Present)(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

HRESULT STDMETHODCALLTYPE DX9_Hook::MyPresentEx(IDirect3DDevice9Ex* _this, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags)
{
    DX9_Hook::Inst()->uses_present = true;
    DX9_Hook::Inst()->prepareForOverlay(_this);
    return (_this->*DX9_Hook::Inst()->PresentEx)(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
}

DX9_Hook::DX9_Hook():
    initialized(false),
    hooked(false),
    uses_present(false),
    EndScene(nullptr),
    Present(nullptr),
    PresentEx(nullptr),
    Reset(nullptr)
{
    _library = LoadLibrary(DX9_DLL);
}

DX9_Hook::~DX9_Hook()
{
    PRINT_DEBUG("DX9 Hook removed\n");

    if (initialized)
    {
        ImGui_ImplDX9_InvalidateDeviceObjects();
        ImGui::DestroyContext();
    }

    FreeLibrary(reinterpret_cast<HMODULE>(_library));

    _inst = nullptr;
}

DX9_Hook* DX9_Hook::Inst()
{
    if( _inst == nullptr )
        _inst = new DX9_Hook;

    return _inst;
}

const char* DX9_Hook::get_lib_name() const
{
    return DX9_DLL;
}

void DX9_Hook::loadFunctions(IDirect3DDevice9* pDevice, bool ex)
{
    void** vTable = *reinterpret_cast<void***>(pDevice);

#define LOAD_FUNC(X) (void*&)X = vTable[(int)IDirect3DDevice9VTable::X]
    LOAD_FUNC(Reset);
    LOAD_FUNC(EndScene);
    LOAD_FUNC(Present);
    if (ex)
        LOAD_FUNC(PresentEx);
#undef LOAD_FUNC
}

#endif//EMU_OVERLAY