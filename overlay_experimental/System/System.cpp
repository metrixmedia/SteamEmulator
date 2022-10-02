/*
 * Copyright (C) Nemirtingas
 * This file is part of System.
 *
 * System is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * System is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with System; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "System.h"
#include "Filesystem.h"
#include "Encoding.hpp"
#include "System_internals.h"

#if defined(SYSTEM_OS_WINDOWS)
    #define WIN32_LEAN_AND_MEAN
    #define VC_EXTRALEAN
    #define NOMINMAX
    #include <Windows.h>
    #include <TlHelp32.h>
    #include <shellapi.h>
    #include <shlobj.h>   // (shell32.lib) Infos about current user folders

    inline bool handle_is_valid(HANDLE h)
    {
        return (h != (HANDLE)0 && h != (HANDLE)-1);
    }

#elif defined(SYSTEM_OS_LINUX) || defined(SYSTEM_OS_APPLE)
    #if defined(SYSTEM_OS_LINUX)
        #include <sys/sysinfo.h> // Get uptime (second resolution)
        #include <dirent.h>
    #else
        #include <sys/sysctl.h>
        #include <mach-o/dyld_images.h>
    #endif

    #include <sys/types.h>
    #include <pwd.h>
    #include <unistd.h>
    #include <dlfcn.h>

#else
    #error "unknown arch"
#endif

#include <fstream>

namespace System {

std::chrono::microseconds GetUpTime()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - GetBootTime());
}

}

namespace System {

#if defined(SYSTEM_OS_WINDOWS)

std::chrono::system_clock::time_point GetBootTime()
{
    static std::chrono::system_clock::time_point boottime(std::chrono::system_clock::now() - std::chrono::milliseconds(GetTickCount64()));
    return boottime;
}

std::vector<std::string> GetProcArgs()
{
    std::vector<std::string> res;

    LPWSTR* szArglist;
    int nArgs;

    szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

    res.reserve(nArgs);
    for (int i = 0; i < nArgs; ++i)
    {
        res.emplace_back(System::Encoding::WCharToUtf8(szArglist[i]));
    }

    LocalFree(szArglist);

    return res;
}

std::string GetEnvVar(std::string const& var)
{
    std::wstring wide(System::Encoding::Utf8ToWChar(var));
    std::wstring wVar;

    DWORD size = GetEnvironmentVariableW(wide.c_str(), nullptr, 0);
    // Size can be 0, and the size includes the null char, so resize to size - 1
    if (size < 2)
        return std::string();

    wVar.resize(size - 1);
    GetEnvironmentVariableW(wide.c_str(), &wVar[0], size);

    return System::Encoding::WCharToUtf8(wVar);
}

std::string GetUserdataPath()
{
    WCHAR szPath[4096] = {};
    HRESULT hr = SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, szPath);

    if (FAILED(hr))
        return std::string();

    return System::Encoding::WCharToUtf8(std::wstring(szPath));
}

std::string GetExecutablePath()
{
    std::string path;
    std::wstring wpath(4096, L'\0');

    wpath.resize(GetModuleFileNameW(nullptr, &wpath[0], wpath.length()));
    return System::Encoding::WCharToUtf8(wpath);
}

std::string GetModulePath()
{
    std::string path;
    std::wstring wpath(4096, L'\0');
    HMODULE hModule;

    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)&GetModulePath, &hModule) != FALSE)
    {
        DWORD size = GetModuleFileNameW((HINSTANCE)hModule, &wpath[0], wpath.length());
        wpath.resize(size);
    }
    return System::Encoding::WCharToUtf8(wpath);
}

std::vector<std::string> GetModules()
{
    std::vector<std::string> paths;
    std::wstring wpath;
    DWORD size;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId(GetCurrentProcess()));
    if (handle_is_valid(hSnap))
    {
        MODULEENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (Module32FirstW(hSnap, &entry) != FALSE)
        {
            wpath.resize(4096);
            size = GetModuleFileNameW((HINSTANCE)entry.hModule, &wpath[0], wpath.length());
            wpath.resize(size);
            paths.emplace_back(System::Encoding::WCharToUtf8(wpath));

            while (Module32NextW(hSnap, &entry) != FALSE)
            {
                wpath.resize(4096);
                size = GetModuleFileNameW((HINSTANCE)entry.hModule, &wpath[0], wpath.length());
                wpath.resize(size);
                paths.emplace_back(System::Encoding::WCharToUtf8(wpath));
            }
        }

        CloseHandle(hSnap);
    }

    return paths;
}

#elif defined(SYSTEM_OS_LINUX) || defined(SYSTEM_OS_APPLE)
#ifdef SYSTEM_OS_LINUX

std::chrono::system_clock::time_point GetBootTime()
{
    static std::chrono::system_clock::time_point boottime(std::chrono::seconds(0));
    if (boottime == std::chrono::system_clock::time_point{})
    {
        std::ifstream uptime_file("/proc/uptime");

        double uptime;
        if (uptime_file)
        {// Get uptime (millisecond resolution)
            uptime_file >> uptime;
            uptime_file.close();
        }
        else
        {// If we can't open /proc/uptime, fallback to sysinfo (second resolution)
            struct sysinfo infos;
            if (sysinfo(&infos) != 0)
                return boottime;

            uptime = infos.uptime;
        }

        std::chrono::system_clock::time_point now_tp = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point uptime_tp(std::chrono::milliseconds(static_cast<uint64_t>(uptime * 1000)));

        boottime = std::chrono::system_clock::time_point(now_tp - uptime_tp);
    }

    return boottime;
}

std::string GetExecutablePath()
{
    std::string exec_path("./");

    char link[2048] = {};
    if (readlink("/proc/self/exe", link, sizeof(link)) > 0)
    {
        exec_path = link;
    }
	
    return exec_path;
}

std::string GetModulePath()
{
    std::string const self("/proc/self/map_files/");
    DIR* dir;
    struct dirent* dir_entry;
    std::string file_path;
    std::string res;
    uint64_t handle = (uint64_t)&GetModulePath;
    uint64_t low, high;
    char* tmp;

    dir = opendir(self.c_str());
    if (dir != nullptr)
    {
        while ((dir_entry = readdir(dir)) != nullptr)
        {
            file_path = dir_entry->d_name;
            if (dir_entry->d_type != DT_LNK)
            {// Not a link
                continue;
            }

            tmp = &file_path[0];
            low = strtoull(tmp, &tmp, 16);
            if ((tmp - file_path.c_str()) < file_path.length())
            {
                high = strtoull(tmp+1, nullptr, 16);
                if (low != 0 && high > low && low <= handle && handle <= high)
                {
                    res = System::ExpandSymlink(self + file_path);
                    break;
                }
            }
        }

        closedir(dir);
    }

    return res;
}

std::vector<std::string> GetModules()
{
    std::string const self("/proc/self/map_files/");
    std::vector<std::string> paths;

    DIR* dir;
    struct dirent* dir_entry;
    std::string path;
    bool found;

    dir = opendir(self.c_str());
    if (dir != nullptr)
    {
        while ((dir_entry = readdir(dir)) != nullptr)
        {
            if (dir_entry->d_type != DT_LNK)
            {// Not a link
                continue;
            }

            found = false;
            path = System::ExpandSymlink(self + dir_entry->d_name);
            for (auto const& item : paths)
            {
                if (item == path)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
                paths.emplace_back(std::move(path));
        }

        closedir(dir);
    }

    return paths;
}

std::vector<std::string> GetProcArgs()
{
    std::vector<std::string> res;
    std::ifstream fcmdline("/proc/self/cmdline", std::ios::in | std::ios::binary);

    if (fcmdline)
    {
        for (std::string line; std::getline(fcmdline, line, '\0');)
        {
            res.emplace_back(std::move(line));
        }
    }

    return res;
}

#else

static int IsProcessTranslated()
{
    int ret = 0;
    size_t size = sizeof(ret);

    // Call the sysctl and if successful return the result
    if (sysctlbyname("sysctl.proc_translated", &ret, &size, NULL, 0) != -1)
        return ret;

    // If "sysctl.proc_translated" is not present then must be native
    if (errno == ENOENT)
        return 0;

    return -1;
}

std::chrono::system_clock::time_point GetBootTime()
{
    static std::chrono::system_clock::time_point boottime{};
    if (boottime == std::chrono::system_clock::time_point{})
    {
        struct timeval boottime_tv;
        size_t len = sizeof(boottime_tv);
        int mib[2] = { CTL_KERN, KERN_BOOTTIME };
        if (sysctl(mib, sizeof(mib)/sizeof(*mib), &boottime_tv, &len, nullptr, 0) < 0)
            return boottime;

        boottime = std::chrono::system_clock::time_point(
            std::chrono::seconds(boottime_tv.tv_sec) +
            std::chrono::microseconds(boottime_tv.tv_usec));
    }

    return boottime;
}

std::string GetExecutablePath()
{
    std::string exec_path("./");

    task_dyld_info dyld_info;
    task_t t;
    pid_t pid = getpid();
    task_for_pid(mach_task_self(), pid, &t);
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;

    if (task_info(t, TASK_DYLD_INFO, reinterpret_cast<task_info_t>(&dyld_info), &count) == KERN_SUCCESS)
    {
        dyld_all_image_infos *dyld_img_infos = reinterpret_cast<dyld_all_image_infos*>(dyld_info.all_image_info_addr);
        if (IsProcessTranslated() == 1)
        {
            for (int i = 0; i < dyld_img_infos->infoArrayCount; ++i)
            {
                exec_path = dyld_img_infos->infoArray[i].imageFilePath;
                if (strcasestr(exec_path.c_str(), "rosetta") != nullptr)
                    continue;

                // In case of a translated process (Rosetta maybe ?), the executable path is not the first entry.
                size_t pos;
                while ((pos = exec_path.find("/./")) != std::string::npos)
                {
                    exec_path.replace(pos, 3, "/");
                }
                break;
            }
        }
        else
        {
            for (int i = 0; i < dyld_img_infos->infoArrayCount; ++i)
            {
                // For now I don't know how to be sure to get the executable path
                // but looks like the 1st entry is the executable path
                exec_path = dyld_img_infos->infoArray[i].imageFilePath;
                size_t pos;
                while ((pos = exec_path.find("/./")) != std::string::npos)
                {
                    exec_path.replace(pos, 3, "/");
                }
                break;
            }
        }
    }

    return exec_path;
}

// Workaround for MacOS, I don't know how to get module path from address.
SYSTEM_EXPORT_API(SYSTEM_EXTERN_C, void, SYSTEM_MODE_EXPORT, SYSTEM_CALL_DEFAULT) GetModulePathPlaceholder() {}

std::string GetModulePath()
{
    task_dyld_info dyld_info;
    task_t t;
    pid_t pid = getpid();
    task_for_pid(mach_task_self(), pid, &t);
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
        
    if (task_info(t, TASK_DYLD_INFO, reinterpret_cast<task_info_t>(&dyld_info), &count) == KERN_SUCCESS)
    {
        dyld_all_image_infos* dyld_img_infos = reinterpret_cast<dyld_all_image_infos*>(dyld_info.all_image_info_addr);
        for (int i = 0; i < dyld_img_infos->infoArrayCount; ++i)
        {
            void* res = dlopen(dyld_img_infos->infoArray[i].imageFilePath, RTLD_NOW);
            if (res != nullptr)
            {
                void* placeholder = dlsym(res, "GetModulePathPlaceholder");
                dlclose(res);
                if(placeholder == (void*)&GetModulePathPlaceholder)
                {
                    std::string res(dyld_img_infos->infoArray[i].imageFilePath);
                    size_t pos;
                    while((pos = res.find("/./")) != std::string::npos)
                    {
                        res.replace(pos, 3, "/");
                    }
                    return res;
                }
            }
        }
    }
    
    return std::string();
}

std::vector<std::string> GetModules()
{
    std::vector<std::string> paths;
    std::string path;
    size_t pos;
    task_dyld_info dyld_info;
    task_t t;
    pid_t pid = getpid();
    task_for_pid(mach_task_self(), pid, &t);
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;

    if (task_info(t, TASK_DYLD_INFO, reinterpret_cast<task_info_t>(&dyld_info), &count) == KERN_SUCCESS)
    {
        dyld_all_image_infos* dyld_img_infos = reinterpret_cast<dyld_all_image_infos*>(dyld_info.all_image_info_addr);
        for (int i = 0; i < dyld_img_infos->infoArrayCount; ++i)
        {
            path = dyld_img_infos->infoArray[i].imageFilePath;
            while ((pos = path.find("/./")) != std::string::npos)
            {
                path.replace(pos, 3, "/");
            }
            paths.emplace_back(std::move(path));
        }
    }

    return paths;
}

std::vector<std::string> GetProcArgs()
{
    std::vector<std::string> res;
    int mib[3];
    int argmax;
    size_t size;
    int nargs;

    mib[0] = CTL_KERN;
    mib[1] = KERN_ARGMAX;

    size = sizeof(argmax);
    if (sysctl(mib, 2, &argmax, &size, NULL, 0) == -1)
    {
        return res;
    }

    std::unique_ptr<char[]> procargs(new char[argmax]);
    if (procargs == nullptr)
    {
        return res;
    }

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROCARGS2;
    mib[2] = getpid();

    size = (size_t)argmax;
    if (sysctl(mib, 3, procargs.get(), &size, NULL, 0) == -1)
    {
        return res;
    }

    memcpy(&nargs, procargs.get(), sizeof(nargs));
    if (nargs <= 0)
    {
        return res;
    }

    char* args_end = procargs.get() + size;
    char* arg_iterator = procargs.get() + sizeof(nargs);
    // Skip saved exec path
    while (*arg_iterator != '\0' && arg_iterator < args_end)
    {
        ++arg_iterator;
    }
    // Skip trailing(s) '\0'
    while (*arg_iterator == '\0' && arg_iterator < args_end)
    {
        ++arg_iterator;
    }

    res.reserve(nargs);
    char* arg = arg_iterator;
    for (int i = 0; i < nargs && arg_iterator < args_end; ++arg_iterator)
    {
        if (*arg_iterator == '\0')
        {
            ++i;
            res.emplace_back(arg);
            arg = arg_iterator + 1;
        }
    }

    return res;
}

#endif

std::string GetUserdataPath()
{
    std::string user_appdata_path;
    /*
    ~/Library/Application Support/<application name>
    ~/Library/Preferences/<application name>
    ~/Library/<application name>/
    */

    struct passwd* user_entry = getpwuid(getuid());
    if (user_entry == nullptr || user_entry->pw_dir == nullptr)
    {
        char* env_var = getenv("HOME");
        if (env_var != nullptr)
        {
            user_appdata_path = env_var;
        }
    }
    else
    {
        user_appdata_path = user_entry->pw_dir;
    }

    if (!user_appdata_path.empty())
    {
#ifdef SYSTEM_OS_LINUX
        user_appdata_path = System::Filesystem::Join(user_appdata_path, ".config");
#else
        user_appdata_path = System::Filesystem::Join(user_appdata_path, "Library", "Application Support");
#endif
    }

    return user_appdata_path;
}

std::string GetEnvVar(std::string const& var)
{
    char* env = getenv(var.c_str());
    if (env == nullptr)
        return std::string();

    return env;
}

#endif

}
