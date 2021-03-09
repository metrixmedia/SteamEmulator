#include "../dll/dll.h"
#include "Hook_Manager.h"

#ifdef EMU_OVERLAY

Hook_Manager::Hook_Manager()
{}

Hook_Manager::~Hook_Manager()
{
    for (auto& i : _hooks)
        delete i;
}

Hook_Manager& Hook_Manager::Inst()
{
    static Hook_Manager hook;
    return hook;
}

void Hook_Manager::AddHook(Base_Hook* hook)
{
    _hooks.insert(hook);
}

void Hook_Manager::RemoveHook(Base_Hook* hook)
{
    auto it = _hooks.find(hook);
    if (it != _hooks.end())
    {
        delete hook;
        _hooks.erase(it);
    }
}

#endif//EMU_OVERLAY