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
 * License along with the System; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include <cstdint>
#include <cstring>
#include <iterator>

#include "StringView.hpp"
#include "StringSwitch.hpp"

namespace System {
namespace String {

///////////////////////////////////////////////////////////
// Implementations

namespace details {
    void LeftTrim(std::string& str);

    void RightTrim(std::string& str);

    void ToUpper(char* str, size_t len);

    void ToLower(char* str, size_t len);

    char* CloneString(System::StringView src);

    size_t CopyString(System::StringView src, char *dst, size_t dst_size);
}


inline void LeftTrim(std::string& str)
{
    details::LeftTrim(str);
}

inline void RightTrim(std::string& str)
{
    details::RightTrim(str);
}

inline void Trim(std::string& str)
{
    LeftTrim(str);
    RightTrim(str);
}

inline std::string CopyLeftTrim(const char* str)
{
    if (str == nullptr)
        return std::string();

    std::string r(str);
    LeftTrim(r);
    return r;
}

inline std::string CopyLeftTrim(System::StringView str)
{
    std::string r(str.to_string());
    LeftTrim(r);
    return r;
}

inline std::string CopyLeftTrim(std::string const& str)
{
    std::string r(str);
    LeftTrim(r);
    return r;
}

inline std::string CopyRightTrim(const char* str)
{
    if (str == nullptr)
        return std::string();

    std::string r(str);
    RightTrim(r);
    return r;
}

inline std::string CopyRightTrim(System::StringView str)
{
    std::string r(str.to_string());
    RightTrim(r);
    return r;
}

inline std::string CopyRightTrim(std::string const& str)
{
    std::string r(str);
    RightTrim(r);
    return r;
}

inline std::string CopyTrim(const char* str)
{
    if (str == nullptr)
        return std::string();

    std::string r(str);
    Trim(r);
    return r;
}

inline std::string CopyTrim(System::StringView str)
{
    std::string r(str.to_string());
    Trim(r);
    return r;
}

inline std::string CopyTrim(std::string const& str)
{
    std::string r(str);
    Trim(r);
    return r;
}

inline void ToLower(std::string& str)
{
    details::ToLower(&str[0], str.length());
}

inline void ToLower(char* str)
{
    if (str == nullptr)
        return;

    details::ToLower(str, strlen(str));
}

inline std::string CopyLower(std::string const& str)
{
    std::string r(str);
    details::ToLower(&r[0], r.length());
    return r;
}

inline std::string CopyLower(const char* str)
{
    std::string r(str == nullptr ? "" : str);
    details::ToLower(&r[0], r.length());
    return r;
}

inline std::string CopyLower(System::StringView str)
{
    std::string r = str.to_string();
    details::ToLower(&r[0], r.length());
    return r;
}

inline void ToUpper(std::string& str)
{
    details::ToUpper(&str[0], str.length());
}

inline void ToUpper(char* str)
{
    if (str == nullptr)
        return;

    details::ToUpper(str, strlen(str));
}

inline std::string CopyUpper(std::string const& str)
{
    std::string r(str);
    details::ToUpper(&r[0], r.length());
    return r;
}

inline std::string CopyUpper(const char* str)
{
    std::string r(str == nullptr ? "" : str);
    details::ToUpper(&r[0], r.length());
    return r;
}

inline std::string CopyUpper(System::StringView str)
{
    std::string r = str.to_string();
    details::ToUpper(&r[0], r.length());
    return r;
}

template<typename IteratorType>
inline std::string Join(IteratorType begin, IteratorType end, const std::string& sep)
{
    std::string res;

    if (begin != end)
        res = *begin++;

    while (begin != end)
    {
        res += sep;
        res += *begin++;
    }

    return res;
}

template<typename T>
inline std::string Join(T const& container, const std::string& sep)
{
    return Join(std::begin(container), std::end(container), sep);
}

// Clone a string allocated with the "new" operator, if str is nullptr, an empty string ("") will be returned, NOT nullptr !
inline char* CloneString(const char* str)
{
    if (str == nullptr)
        return details::CloneString(System::StringView(""));

    return details::CloneString(System::StringView(str, strlen(str)));
}

inline char* CloneString(std::string const& str)
{
    return details::CloneString(System::StringView(str));
}

inline char* CloneString(System::StringView str)
{
    return details::CloneString(str);
}

// Will always end the C-String with a null char.
inline size_t CopyString(const char* src, char* dst, size_t dst_size)
{
    if (src == nullptr)
        return details::CopyString(System::StringView(""), dst, dst_size);

    return details::CopyString(System::StringView(src, strlen(src)), dst, dst_size);
}

inline size_t CopyString(System::StringView src, char* dst, size_t dst_size)
{
    return details::CopyString(src, dst, dst_size);
}

inline size_t CopyString(std::string const& src, char* dst, size_t dst_size) 
{
    return details::CopyString(System::StringView(src), dst, dst_size);
}

template<size_t N>
inline size_t CopyString(const char* src, char(&dst)[N])
{
    if (src == nullptr)
        return details::CopyString(System::StringView(""), dst, N);

    return details::CopyString(System::StringView(src, strlen(src)), dst, N);
}

template<size_t N>
inline size_t CopyString(System::StringView src, char(&dst)[N])
{
    return details::CopyString(src, dst, N);
}

template<size_t N>
inline size_t CopyString(std::string const& src, char(&dst)[N])
{
    return details::CopyString(System::StringView(src), dst, N);
}

}
}