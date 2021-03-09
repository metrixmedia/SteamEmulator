#ifndef __INCLUDED_OPENGLX_HOOK_H__
#define __INCLUDED_OPENGLX_HOOK_H__

#include "../Base_Hook.h"

#ifdef __LINUX__
#ifdef EMU_OVERLAY
#include <GL/glew.h>
#include <GL/glx.h>

class OpenGLX_Hook : public Base_Hook
{
public:
    static constexpr const char *DLL_NAME = "libGLX.so";

private:
    static OpenGLX_Hook* _inst;

    // Variables
    bool hooked;
    bool initialized;
    Display *display;
    GLXContext context;

    // Functions
    OpenGLX_Hook();

    void resetRenderState();
    void prepareForOverlay(Display* display, GLXDrawable drawable);

    // Hook to render functions


    decltype(glXSwapBuffers)* _glXSwapBuffers;

public:
    static void MyglXSwapBuffers(Display* display, GLXDrawable drawable);

    virtual ~OpenGLX_Hook();

    bool start_hook();
    static OpenGLX_Hook* Inst();
    virtual const char* get_lib_name() const;
    void loadFunctions(decltype(glXSwapBuffers)* pfnglXSwapBuffers);
};

#endif//EMU_OVERLAY
#endif//__LINUX__
#endif//__INCLUDED_OPENGLX_HOOK_H__
