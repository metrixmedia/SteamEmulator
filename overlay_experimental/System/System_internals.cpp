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

#include "System_internals.h"

#if defined(SYSTEM_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <Windows.h>

namespace System {

}

#elif defined(SYSTEM_OS_LINUX)
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace System {
SYSTEM_HIDE_API(std::string, SYSTEM_CALL_DEFAULT) ExpandSymlink(std::string file_path)
{
    struct stat file_stat;
    std::string link_target;
    ssize_t name_len = 128;
    while(lstat(file_path.c_str(), &file_stat) >= 0 && S_ISLNK(file_stat.st_mode) == 1)
    {
        do
        {
            name_len *= 2;
            link_target.resize(name_len);
            name_len = readlink(file_path.c_str(), &link_target[0], link_target.length());
        } while (name_len == link_target.length());
        link_target.resize(name_len);
        file_path = std::move(link_target);
    }

    return file_path;
}

}

#endif
