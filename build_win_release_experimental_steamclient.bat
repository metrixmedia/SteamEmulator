@echo off
cd /d "%~dp0"
mkdir release\experimental_steamclient
del /Q release\experimental_steamclient\*
call build_set_protobuf_directories.bat
"%PROTOC_X86_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
call build_env_x86.bat
cl dll/rtlgenrandom.c dll/rtlgenrandom.def
cl /LD /DEMU_RELEASE_BUILD /DEMU_EXPERIMENTAL_BUILD /DSTEAMCLIENT_DLL /DCONTROLLER_SUPPORT /DEMU_OVERLAY /DGLEW_STATIC /IImGui /Iglew\include /DNDEBUG /I%PROTOBUF_X86_DIRECTORY%\include\ dll/*.cpp dll/*.cc detours/*.cpp controller/gamepad.c ImGui/*.cpp ImGui/impls/*.cpp ImGui/impls/windows/*.cpp overlay_experimental/*.cpp overlay_experimental/windows/*.cpp "%PROTOBUF_X86_LIBRARY%" glew\glew.c opengl32.lib Iphlpapi.lib Ws2_32.lib rtlgenrandom.lib Shell32.lib Winmm.lib /EHsc /MP12 /Ox /link /debug:none /OUT:release\experimental_steamclient\steamclient.dll
"%PROTOC_X64_EXE%" -I.\dll\ --cpp_out=.\dll\ .\dll\net.proto
cl steamclient_loader/*.cpp advapi32.lib user32.lib /EHsc /MP12 /Ox /link /debug:none /OUT:release\experimental_steamclient\steamclient_loader.exe
copy steamclient_loader\ColdClientLoader.ini release\experimental_steamclient\
call build_env_x64.bat
cl dll/rtlgenrandom.c dll/rtlgenrandom.def
cl /LD /DEMU_RELEASE_BUILD /DEMU_EXPERIMENTAL_BUILD /DSTEAMCLIENT_DLL /DCONTROLLER_SUPPORT /DEMU_OVERLAY /DGLEW_STATIC /IImGui /Iglew\include /DNDEBUG /I%PROTOBUF_X64_DIRECTORY%\include\ dll/*.cpp dll/*.cc detours/*.cpp controller/gamepad.c ImGui/*.cpp ImGui/impls/*.cpp ImGui/impls/windows/*.cpp overlay_experimental/*.cpp overlay_experimental/windows/*.cpp "%PROTOBUF_X64_LIBRARY%" glew\glew.c opengl32.lib Iphlpapi.lib Ws2_32.lib rtlgenrandom.lib Shell32.lib Winmm.lib /EHsc /MP12 /Ox /link /debug:none /OUT:release\experimental_steamclient\steamclient64.dll
copy Readme_experimental_steamclient.txt release\experimental_steamclient\Readme.txt
