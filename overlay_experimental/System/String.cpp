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

#include "String.hpp"

#include <string.h>
#include <algorithm>
#include <cctype>

namespace System {
namespace String {
namespace details {

void LeftTrim(std::string& str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](const char c)
    {
        return !std::isspace((unsigned char)c);
    }));
}

void RightTrim(std::string& str)
{
    str.erase(std::find_if(str.rbegin(), str.rend(), [](const char c)
    {
        return !std::isspace((unsigned char)c);
    }).base(), str.end());
}

void ToUpper(char* str, size_t len)
{
    while(len--)
    {
        unsigned char c = (unsigned char)*str;
        *str++ = std::toupper(c);
    }
}

void ToLower(char* str, size_t len)
{
    while (len--)
    {
        unsigned char c = (unsigned char)*str;
        *str++ = std::tolower(c);
    }
}

char* CloneString(System::StringView src)
{
    size_t len = src.length() + 1;
    char* res = new char[len];
    memcpy(res, src.data(), len);
    return res;
}

size_t CopyString(System::StringView src, char* dst, size_t dst_size)
{
    size_t written = 0;
    if (dst != nullptr && dst_size > 0)
    {
        written = src.length() > dst_size ? dst_size - 1 : src.length();
        memcpy(dst, src.data(), written);
        dst[written] = '\0';
    }
    return written;
}

}// namespace details

}// namespace String
}// namespace System