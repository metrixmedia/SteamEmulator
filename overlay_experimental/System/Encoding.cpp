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

#include "Encoding.hpp"
#include "utfcpp/utf8.h"

#include <string.h>
#include <algorithm>

namespace System {
namespace Encoding {

namespace details {
    template<typename T, size_t s = sizeof(typename T::value_type)>
    struct string_deducer
    {
        static std::string  convert_string(T const& str) = delete;
        static std::wstring convert_wstring(T const& str) = delete;
    };

    template<typename T>
    struct string_deducer<T, 1>
    {
        static std::string convert_string(T const& str)
        {
            return std::string(std::begin(str), std::end(str));
        }

        static std::wstring convert_wstring(std::string const& str)
        {
            std::wstring r(std::begin(str), std::end(str));
            return r;
        }
    };

    template<typename T>
    struct string_deducer<T, 2>
    {
        static std::string convert_string(T const& str)
        {
            std::string r;
            utf8::utf16to8(std::begin(str), std::end(str), std::back_inserter(r));
            return r;
        }

        static std::wstring convert_wstring(std::string const& str)
        {
            std::wstring r;
            utf8::utf8to16(std::begin(str), std::end(str), std::back_inserter(r));
            return r;
        }
    };

    template<typename T>
    struct string_deducer<T, 4>
    {
        static std::string convert_string(T const& str)
        {
            std::string r;
            utf8::utf32to8(std::begin(str), std::end(str), std::back_inserter(r));
            return r;
        }

        static std::wstring convert_wstring(std::string const& str)
        {
            std::wstring r;
            utf8::utf8to32(std::begin(str), std::end(str), std::back_inserter(r));
            return r;
        }
    };
}

std::wstring Utf8ToWChar(std::string const& str)
{
    return details::string_deducer<std::wstring>::convert_wstring(str);
}

std::u16string Utf8ToUtf16(std::string const& str)
{
    return utf8::utf8to16(str);
}

std::u32string Utf8ToUtf32(std::string const& str)
{
    return utf8::utf8to32(str);
}

std::string WCharToUtf8(std::wstring const& str)
{
    return details::string_deducer<std::wstring>::convert_string(str);
}

std::string Utf16ToUtf8(std::u16string const& str)
{
    return utf8::utf16to8(str);
}

std::string Utf32ToUtf8(std::u32string const& str)
{
    return utf8::utf32to8(str);
}

size_t EncodedLength(std::string const& str)
{
    return utf8::distance(str.begin(), str.end());
}

}// namespace Encoding
}// namespace System