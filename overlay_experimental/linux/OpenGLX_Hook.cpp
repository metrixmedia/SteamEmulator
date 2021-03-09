#include "OpenGLX_Hook.h"
#include "X11_Hook.h"
#include "../Renderer_Detector.h"
#include "../dll/dll.h"

#ifdef __LINUX__
#ifdef EMU_OVERLAY

#include <imgui.h>
#include <impls/imgui_impl_opengl3.h>

#include "../steam_overlay.h"

OpenGLX_Hook* OpenGLX_Hook::_inst = nullptr;

bool OpenGLX_Hook::start_hook()
{
    bool res = true;
    if (!hooked)
    {
        if (!X11_Hook::Inst()->start_hook())
            return false;

        GLenum err = glewInit();

        if (err == GLEW_OK)
        {
            PRINT_DEBUG("Hooked OpenGLX\n");

            hooked = true;
            Renderer_Detector::Inst().renderer_found(this);

            /*
            UnhookAll();
            BeginHook();
            HookFuncs(
                std::make_pair<void**, void*>(&(void*&)_glXSwapBuffers, (void*)&OpenGLX_Hook::MyglXSwapBuffers)
            );
            EndHook();
            */

            get_steam_client()->steam_overlay->HookReady();
        }
        else
        {
            PRINT_DEBUG("Failed to hook OpenGLX\n");
            /* Problem: glewInit failed, something is seriously wrong. */
            PRINT_DEBUG("Error: %s\n", glewGetErrorString(err));
            res = false;
        }
    }
    return true;
}

void OpenGLX_Hook::resetRenderState()
{
    if (initialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        X11_Hook::Inst()->resetRenderState();
        ImGui::DestroyContext();

        glXDestroyContext(display, context);
        display = nullptr;
        initialized = false;
    }
}

// Try to make this function and overlay's proc as short as possible or it might affect game's fps.
void OpenGLX_Hook::prepareForOverlay(Display* display, GLXDrawable drawable)
{
    PRINT_DEBUG("Called SwapBuffer hook");

    if( (Window)drawable != X11_Hook::Inst()->get_game_wnd() )
        X11_Hook::Inst()->resetRenderState();

    if( ! initialized )
    {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL;

        ImGui_ImplOpenGL3_Init();

        int attributes[] = { //can't be const b/c X11 doesn't like it.  Not sure if that's intentional or just stupid.
            GLX_RGBA, //apparently nothing comes after this?
            GLX_RED_SIZE,    8,
            GLX_GREEN_SIZE,  8,
            GLX_BLUE_SIZE,   8,
            GLX_ALPHA_SIZE,  8,
            //Ideally, the size would be 32 (or at least 24), but I have actually seen
            //  this size (on a modern OS even).
            GLX_DEPTH_SIZE, 16,
            GLX_DOUBLEBUFFER, True,
            None
        };

        XVisualInfo* visual_info = glXChooseVisual(display, DefaultScreen(display), attributes);

        context = glXCreateContext(display, visual_info, nullptr, True);
        this->display = display;

        get_steam_client()->steam_overlay->CreateFonts();

        initialized = true;
    }

    auto oldContext = glXGetCurrentContext();

    glXMakeCurrent(display, drawable, context);

    ImGui_ImplOpenGL3_NewFrame();
    X11_Hook::Inst()->prepareForOverlay(display, (Window)drawable);

    ImGui::NewFrame();

    get_steam_client()->steam_overlay->OverlayProc();

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glXMakeCurrent(display, drawable, oldContext);
}

void OpenGLX_Hook::MyglXSwapBuffers(Display* display, GLXDrawable drawable)
{
    OpenGLX_Hook::Inst()->prepareForOverlay(display, drawable);
    OpenGLX_Hook::Inst()->_glXSwapBuffers(display, drawable);
}

OpenGLX_Hook::OpenGLX_Hook():
    initialized(false),
    hooked(false),
    _glXSwapBuffers(nullptr)
{
    //_library = dlopen(DLL_NAME);
}

OpenGLX_Hook::~OpenGLX_Hook()
{
    PRINT_DEBUG("OpenGLX Hook removed\n");

    if (initialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
        glXDestroyContext(display, context);
    }

    //dlclose(_library);

    _inst = nullptr;
}

OpenGLX_Hook* OpenGLX_Hook::Inst()
{
    if (_inst == nullptr)
        _inst = new OpenGLX_Hook;

    return _inst;
}

const char* OpenGLX_Hook::get_lib_name() const
{
    return DLL_NAME;
}

void OpenGLX_Hook::loadFunctions(decltype(glXSwapBuffers)* pfnglXSwapBuffers)
{
    _glXSwapBuffers = pfnglXSwapBuffers;
}

#endif//EMU_OVERLAY
#endif//__LINUX__
