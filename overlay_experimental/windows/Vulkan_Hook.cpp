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

#include "Vulkan_Hook.h"
#include "Windows_Hook.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

Vulkan_Hook* Vulkan_Hook::_inst = nullptr;

bool Vulkan_Hook::StartHook(std::function<bool(bool)> key_combination_callback, std::set<ingame_overlay::ToggleKey> toggle_keys)
{
    SPDLOG_WARN("Vulkan overlay is not yet supported.");
    return false;
    if (!_Hooked)
    {
        if (vkQueuePresentKHR == nullptr)
        {
            SPDLOG_WARN("Failed to hook Vulkan: Rendering functions missing.");
            return false;
        }

        if (!Windows_Hook::Inst()->StartHook(key_combination_callback, toggle_keys))
            return false;

        _WindowsHooked = true;

        SPDLOG_INFO("Hooked Vulkan");
        _Hooked = true;

        BeginHook();
        HookFuncs(
            std::make_pair<void**, void*>(&(PVOID&)vkQueuePresentKHR, &Vulkan_Hook::MyvkQueuePresentKHR)
        );
        EndHook();
    }
    return true;
}

bool Vulkan_Hook::IsStarted()
{
    return _Hooked;
}

void Vulkan_Hook::_ResetRenderState()
{
}

// Try to make this function and overlay's proc as short as possible or it might affect game's fps.
void Vulkan_Hook::_PrepareForOverlay()
{
    
    
}

VKAPI_ATTR VkResult VKAPI_CALL Vulkan_Hook::MyvkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
{
    auto inst = Vulkan_Hook::Inst();
    inst->_PrepareForOverlay();
    return inst->vkQueuePresentKHR(queue, pPresentInfo);
}

Vulkan_Hook::Vulkan_Hook():
    _Hooked(false),
    _WindowsHooked(false),
    _Initialized(false),
    vkQueuePresentKHR(nullptr)
{
}

Vulkan_Hook::~Vulkan_Hook()
{
    SPDLOG_INFO("Vulkan_Hook Hook removed");

    if (_WindowsHooked)
        delete Windows_Hook::Inst();

    if (_Initialized)
    {
    }

    _inst = nullptr;
}

Vulkan_Hook* Vulkan_Hook::Inst()
{
    if (_inst == nullptr)
        _inst = new Vulkan_Hook;

    return _inst;
}

std::string Vulkan_Hook::GetLibraryName() const
{
    return LibraryName;
}

void Vulkan_Hook::LoadFunctions(decltype(::vkQueuePresentKHR)* _vkQueuePresentKHR)
{
    vkQueuePresentKHR = _vkQueuePresentKHR;
}

std::weak_ptr<uint64_t> Vulkan_Hook::CreateImageResource(const void* image_data, uint32_t width, uint32_t height)
{
    return std::shared_ptr<uint64_t>(nullptr);
}

void Vulkan_Hook::ReleaseImageResource(std::weak_ptr<uint64_t> resource)
{

}