#ifndef __INCLUDED_HOOK_BASE_H__
#define __INCLUDED_HOOK_BASE_H__

#include "Base_Hook.h"

#ifdef EMU_OVERLAY

class Hook_Manager
{
    friend class Base_Hook;

protected:
    std::set<Base_Hook*> _hooks; 

    Hook_Manager();
    virtual ~Hook_Manager();

public:
    static Hook_Manager& Inst();

    void AddHook(Base_Hook* hook);
    void RemoveHook(Base_Hook* hook);
};

#endif//EMU_OVERLAY

#endif//__INCLUDED_HOOK_BASE_H__