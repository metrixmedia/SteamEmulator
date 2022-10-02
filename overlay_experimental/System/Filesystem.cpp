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

#include "Filesystem.h"
#include "Encoding.hpp"
#include "System_internals.h"

#if defined(SYSTEM_OS_WINDOWS)
    #define WIN32_LEAN_AND_MEAN
    #define VC_EXTRALEAN
    #define NOMINMAX
    #include <Windows.h>

    #ifdef CreateDirectory
    #undef CreateDirectory
    #endif
    #ifdef DeleteFile
    #undef DeleteFile
    #endif

#elif defined(SYSTEM_OS_LINUX) || defined(SYSTEM_OS_APPLE)
    #include <sys/types.h>
    #include <sys/ioctl.h> // get iface broadcast
    #include <sys/stat.h>  // stats on a file (is directory, size, mtime)

    #include <dirent.h> // to open directories
    #include <dlfcn.h>  // dlopen (like dll for linux)

    #include <string.h>
    #include <limits.h> // PATH_MAX
    #include <unistd.h>

#else
    #error "unknown arch"
#endif

#include <fstream>
#include <algorithm>
#include <iterator>

#include <ctime>

namespace System {
namespace Filesystem {

static void _CleanSlashes(std::string& str);

std::string Filename(std::string const& path)
{
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos)
        return path.substr(pos+1);

    return path;
}

std::string Dirname(std::string const& path)
{
    std::string r(path);
    _CleanSlashes(r);
    size_t pos = r.find_last_of("/\\");

    if (pos == std::string::npos || (pos == 0 && r.length() == 1))
        return std::string();

    if (pos == 0)
        ++pos;

    return r.substr(0, pos);
}

std::string Join(StringView r, StringView l)
{
    std::string result(r.to_string());

    result += Separator;
    result += l.to_string();

    _CleanSlashes(result);
    return result;
}

std::string CanonicalPath(std::string const& path)
{
    if (IsAbsolute(path))
        return CleanPath(path);

    return CleanPath(Join(GetCwd(),path));
}

size_t FileSize(std::string const& path)
{
    std::ifstream in_file(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (in_file)
    {
        return in_file.tellg();
    }

    return 0;
}

std::chrono::system_clock::time_point FileATime(std::string const& path)
{
    struct stat file_stat = {};
    if (stat(path.c_str(), &file_stat) != 0)
        return std::chrono::system_clock::time_point{};

    return std::chrono::system_clock::from_time_t(file_stat.st_atime);
}

std::chrono::system_clock::time_point FileMTime(std::string const& path)
{
    struct stat file_stat = {};
    if (stat(path.c_str(), &file_stat) != 0)
        return std::chrono::system_clock::time_point{};

    return std::chrono::system_clock::from_time_t(file_stat.st_mtime);
}

std::chrono::system_clock::time_point FileCTime(std::string const& path)
{
    struct stat file_stat = {};
    if (stat(path.c_str(), &file_stat) != 0)
        return std::chrono::system_clock::time_point{};

    return std::chrono::system_clock::from_time_t(file_stat.st_ctime);
}

#ifdef SYSTEM_OS_WINDOWS

static void _CleanSlashes(std::string& str)
{
    size_t pos;
    std::replace(str.begin(), str.end(), '/', '\\');

    while ((pos = str.find("\\\\")) != std::string::npos)
        str.replace(pos, 2, "\\");

    pos = 0;
    while ((pos = str.find("\\.", pos)) != std::string::npos)
    {
        if (str[pos + 2] == '\\' || (pos + 2) >= str.length())
        {
            str.replace(pos, 3, "\\");
        }
        else
        {
            ++pos;
        }
    }
}

std::string GetCwd()
{
    DWORD size = GetCurrentDirectoryW(0, nullptr);
    if (size == 0)
        return ".";

    std::wstring wdirectory;
    ++size;
    wdirectory.resize(size);
    wdirectory.resize(GetCurrentDirectoryW(size, &wdirectory[0]));
    wdirectory += L'\\';

    return System::Encoding::WCharToUtf8(wdirectory);
}

bool IsAbsolute(std::string const& path)
{
    return path.length() >= 2 && (((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z')) && path[1] == ':');
}

std::string CleanPath(std::string const& path)
{
    std::string cleaned_path(path);
    size_t pos;
    size_t size;

    _CleanSlashes(cleaned_path);

    pos = 0;
    while ((pos = cleaned_path.find("\\..", pos)) != std::string::npos )
    {
        if (cleaned_path[pos + 3] == '\\' || (pos+3) >= cleaned_path.length())
        {
            if (pos == 0)
                size = 3;
            else
            {
                size_t parent_pos = cleaned_path.rfind("\\", pos - 1);
                if (parent_pos == std::string::npos)
                {
                    size = pos + 3;
                    pos = 0;
                }
                else
                {
                    size = 3 + pos - parent_pos;
                    pos = parent_pos;
                }
            }

            cleaned_path.replace(pos, size, "");
        }
        else
        {
            ++pos;
        }
    }

    return cleaned_path;
}

bool IsDir(std::string const& path)
{
    std::wstring wpath(System::Encoding::Utf8ToWChar(path));

    DWORD attrs = GetFileAttributesW(wpath.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && attrs & FILE_ATTRIBUTE_DIRECTORY;
}

bool IsFile(std::string const& path)
{
    std::wstring wpath(System::Encoding::Utf8ToWChar(path));

    DWORD attrs = GetFileAttributesW(wpath.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && ((attrs & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY);
}

bool Exists(std::string const& path)
{
    std::wstring wpath(System::Encoding::Utf8ToWChar(path));

    DWORD attrs = GetFileAttributesW(wpath.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES;
}

bool CreateDirectory(std::string const& directory, bool recursive)
{
    size_t pos = 0;
    struct _stat sb;

    std::wstring sub_dir;
    std::wstring wdirectory(System::Encoding::Utf8ToWChar(directory));

    if (wdirectory.empty())
        return false;

    if (recursive)
    {
        pos = 3;

        do
        {
            pos = wdirectory.find_first_of(L"\\/", pos + 1);
            sub_dir = std::move(wdirectory.substr(0, pos));
            if (_wstat(sub_dir.c_str(), &sb) == 0)
            {
                if (!(sb.st_mode & _S_IFDIR))
                {// A subpath in the target is not a directory
                    return false;
                }
                // Folder Exists
            }
            else if (CreateDirectoryW(wdirectory.substr(0, pos).c_str(), NULL) == FALSE && GetLastError() != ERROR_ALREADY_EXISTS)
            {// Failed to create directory
                return false;
            }
        }
        while (pos != std::string::npos);

        return true;
    }
    
    return (CreateDirectoryW(wdirectory.c_str(), NULL) != FALSE || GetLastError() == ERROR_ALREADY_EXISTS);
}

bool DeleteFile(std::string const& path)
{
    std::wstring wpath(System::Encoding::Utf8ToWChar(path));
    return DeleteFileW(wpath.c_str()) == TRUE || GetLastError() == ERROR_FILE_NOT_FOUND;
}

static std::vector<std::wstring> ListFiles(std::wstring const& path, bool files_only, bool recursive)
{
    std::vector<std::wstring> files;
    WIN32_FIND_DATAW hfind_data;
    HANDLE hfind = INVALID_HANDLE_VALUE;

    std::wstring search_path = path;

    if (*path.rbegin() != L'\\')
        search_path += L'\\';

    search_path += L'*';

    // Start iterating over the files in the path directory.
    hfind = FindFirstFileW(search_path.c_str(), &hfind_data);
    if (hfind != INVALID_HANDLE_VALUE)
    {
        search_path.pop_back();
        do // Managed to locate and create an handle to that folder.
        {
            if (wcscmp(L".", hfind_data.cFileName) == 0
                || wcscmp(L"..", hfind_data.cFileName) == 0)
                continue;

            if (hfind_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (recursive)
                {
                    std::wstring dir_name = hfind_data.cFileName;

                    std::vector<std::wstring> sub_files = std::move(ListFiles(search_path + dir_name, files_only, true));
                    std::transform(sub_files.begin(), sub_files.end(), std::back_inserter(files), [&dir_name](std::wstring& Filename)
                    {
                        return dir_name + L'\\' + Filename;
                    });
                }
                if (!files_only)
                {
                    files.emplace_back(hfind_data.cFileName);
                }
            }
            else
            {
                files.emplace_back(hfind_data.cFileName);
            }
        } while (FindNextFileW(hfind, &hfind_data) == TRUE);
        FindClose(hfind);
    }

    return files;
}

std::vector<std::string> ListFiles(std::string const& path, bool files_only, bool recursive)
{
    std::vector<std::string> files;
    std::wstring wpath(System::Encoding::Utf8ToWChar(path));

    std::vector<std::wstring> wfiles(std::move(ListFiles(wpath, files_only, recursive)));

    files.reserve(wfiles.size());
    std::transform(wfiles.begin(), wfiles.end(), std::back_inserter(files), [](std::wstring const& wFilename)
    {
        return System::Encoding::WCharToUtf8(wFilename);
    });

    return files;
}

#else

static void _CleanSlashes(std::string& str)
{
    size_t pos;
    std::replace(str.begin(), str.end(), '\\', '/');

    while ((pos = str.find("//")) != std::string::npos)
        str.replace(pos, 2, "/");

    pos = 0;
    while ((pos = str.find("/.", pos)) != std::string::npos)
    {
        if (str[pos + 2] == '/' || (pos + 2) >= str.length())
        {
            str.replace(pos, 3, "/");
        }
        else
        {
            ++pos;
        }
    }
}

std::string GetCwd()
{
    char buff[4096];
    std::string tmp(getcwd(buff, 4096) == nullptr ? "." : buff);

    tmp += '/';
    return tmp;
}

bool IsAbsolute(std::string const& path)
{
    return path[0] == '/';
}

std::string CleanPath(std::string const& path)
{
    std::string cleaned_path(path);
    size_t pos;
    size_t size;

    std::replace(cleaned_path.begin(), cleaned_path.end(), '\\', '/');

    _CleanSlashes(cleaned_path);

    pos = 0;
    while ((pos = cleaned_path.find("/..", pos)) != std::string::npos)
    {
        if (cleaned_path[pos + 3] == '/' || (pos + 3) >= cleaned_path.length())
        {
            if (pos == 0)
                size = 3;
            else
            {
                size_t parent_pos = cleaned_path.rfind("/", pos - 1);
                if (parent_pos == std::string::npos)
                {
                    size = pos + 3;
                    pos = 0;
                }
                else
                {
                    size = 3 + pos - parent_pos;
                    pos = parent_pos;
                }
            }

            cleaned_path.replace(pos, size, "");
        }
        else
        {
            ++pos;
        }
    }

    return cleaned_path;
}

bool IsDir(std::string const& path)
{
    struct stat sb;
    if (stat(path.c_str(), &sb) == 0)
    {
        return S_ISDIR(sb.st_mode);
    }

    return false;
}

bool IsFile(std::string const& path)
{
    struct stat sb;
    if (stat(path.c_str(), &sb) == 0)
    {
        return S_ISREG(sb.st_mode);
    }

    return false;
}

bool Exists(std::string const& path)
{
    struct stat sb;
    return stat(path.c_str(), &sb) == 0;
}

bool CreateDirectory(std::string const& directory, bool recursive)
{
    size_t pos = 0;
    struct stat sb;

    std::string sub_dir;

    do
    {
        pos = directory.find("/", pos + 1);
        sub_dir = std::move(directory.substr(0, pos));
        if (stat(sub_dir.c_str(), &sb) == 0)
        {
            if (!S_ISDIR(sb.st_mode))
            {// A subpath in the target is not a directory
                return false;
            }
            // Folder Exists
        }
        else if (mkdir(sub_dir.c_str(), 0755) < 0 && errno != EEXIST)
        {// Failed to create directory (no permission?)
            return false;
        }
    }
    while (pos != std::string::npos);

    return true;
}

bool DeleteFile(std::string const& path)
{
    return unlink(path.c_str()) == 0;
}

std::vector<std::string> ListFiles(std::string const& path, bool files_only, bool recursive)
{
    std::vector<std::string> files;

    std::string search_path = path;

    if (*path.rbegin() != Separator)
        search_path += Separator;

    DIR* dir = opendir(search_path.c_str());
    struct dirent* entry;

    if (dir == nullptr)
        return files;

    while ((entry = readdir(dir)) != nullptr)
    {
        if (strcmp(entry->d_name, ".") == 0
            || strcmp(entry->d_name, "..") == 0)
            continue;

        if (entry->d_type == DT_DIR)
        {
            if (recursive)
            {
                std::string dir_name = entry->d_name;
                std::vector<std::string> sub_files = std::move(ListFiles(search_path + dir_name, true));
                std::transform(sub_files.begin(), sub_files.end(), std::back_inserter(files), [&dir_name](std::string& Filename)
                {
                    return dir_name + Separator + Filename;
                });
            }
            if (!files_only)
            {
                files.emplace_back(entry->d_name);
            }
        }
        else if (entry->d_type == DT_REG)
        {
            files.emplace_back(entry->d_name);
        }
    }

    closedir(dir);

    return files;
}

#endif

}
}
