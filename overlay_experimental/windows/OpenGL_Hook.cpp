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

#include "OpenGL_Hook.h"
#include "Windows_Hook.h"

#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>

#include <glad/gl.h>

OpenGL_Hook* OpenGL_Hook::_inst = nullptr;

bool OpenGL_Hook::StartHook(std::function<bool(bool)> key_combination_callback, std::set<ingame_overlay::ToggleKey> toggle_keys)
{
    if (!_Hooked)
    {
        if (wglSwapBuffers == nullptr)
        {
            SPDLOG_WARN("Failed to hook OpenGL: Rendering functions missing.");
            return false;
        }

        if (!Windows_Hook::Inst()->StartHook(key_combination_callback, toggle_keys))
            return false;

        _WindowsHooked = true;

        SPDLOG_INFO("Hooked OpenGL");

        _Hooked = true;

        UnhookAll();
        BeginHook();
        HookFuncs(
            std::make_pair<void**, void*>(&(PVOID&)wglSwapBuffers, &OpenGL_Hook::MywglSwapBuffers)
        );
        EndHook();
    }
    return true;
}

bool OpenGL_Hook::IsStarted()
{
    return _Hooked;
}

void OpenGL_Hook::_ResetRenderState()
{
    if (_Initialized)
    {
        OverlayHookReady(false);

        ImGui_ImplOpenGL3_Shutdown();
        Windows_Hook::Inst()->ResetRenderState();
        ImGui::DestroyContext();

        _LastWindow = nullptr;
        _Initialized = false;
    }
}

// Try to make this function and overlay's proc as short as possible or it might affect game's fps.
void OpenGL_Hook::_PrepareForOverlay(HDC hDC)
{
    HWND hWnd = WindowFromDC(hDC);

    if (hWnd != _LastWindow)
        _ResetRenderState();

    if (!_Initialized)
    {
        ImGui::CreateContext((ImFontAtlas *)ImGuiFontAtlas);
        ImGui_ImplOpenGL3_Init();

        _LastWindow = hWnd;

        Windows_Hook::Inst()->SetInitialWindowSize(hWnd);

        _Initialized = true;
        OverlayHookReady(true);
    }

    if (ImGui_ImplOpenGL3_NewFrame() && Windows_Hook::Inst()->PrepareForOverlay(hWnd))
    {
        ImGui::NewFrame();

        OverlayProc();

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}

BOOL WINAPI OpenGL_Hook::MywglSwapBuffers(HDC hDC)
{
    auto inst = OpenGL_Hook::Inst();
    inst->_PrepareForOverlay(hDC);
    return inst->wglSwapBuffers(hDC);
}

OpenGL_Hook::OpenGL_Hook():
    _Hooked(false),
    _WindowsHooked(false),
    _Initialized(false),
    _LastWindow(nullptr),
    wglSwapBuffers(nullptr)
{
}

OpenGL_Hook::~OpenGL_Hook()
{
    SPDLOG_INFO("OpenGL Hook removed");

    if (_WindowsHooked)
        delete Windows_Hook::Inst();

    if (_Initialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }

    _inst = nullptr;
}

OpenGL_Hook* OpenGL_Hook::Inst()
{
    if (_inst == nullptr)
        _inst = new OpenGL_Hook;

    return _inst;
}

std::string OpenGL_Hook::GetLibraryName() const
{
    return LibraryName;
}

void OpenGL_Hook::LoadFunctions(wglSwapBuffers_t pfnwglSwapBuffers)
{
    wglSwapBuffers = pfnwglSwapBuffers;
}

std::weak_ptr<uint64_t> OpenGL_Hook::CreateImageResource(const void* image_data, uint32_t width, uint32_t height)
{
    GLuint* texture = new GLuint(0);
    glGenTextures(1, texture);
    if (glGetError() != GL_NO_ERROR)
    {
        delete texture;
        return std::shared_ptr<uint64_t>(nullptr);
    }
    
    // Save old texture id
    GLint oldTex;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTex);

    glBindTexture(GL_TEXTURE_2D, *texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

    glBindTexture(GL_TEXTURE_2D, oldTex);

    auto ptr = std::shared_ptr<uint64_t>((uint64_t*)texture, [](uint64_t* handle)
    {
        if (handle != nullptr)
        {
            GLuint* texture = (GLuint*)handle;
            glDeleteTextures(1, texture);
            delete texture;
        }
    });

    _ImageResources.emplace(ptr);
    return ptr;
}

void OpenGL_Hook::ReleaseImageResource(std::weak_ptr<uint64_t> resource)
{
    auto ptr = resource.lock();
    if (ptr)
    {
        auto it = _ImageResources.find(ptr);
        if (it != _ImageResources.end())
            _ImageResources.erase(it);
    }
}