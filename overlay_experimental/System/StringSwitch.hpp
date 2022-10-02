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

#pragma once

#include <cstdint>
#include <string>

#include "SystemDetector.h"
#include "StringView.hpp"

namespace System {

namespace StringSwitch {

#if defined(SYSTEM_ARCH_X86)
    using hash_type = uint32_t;
#else
    using hash_type = uint64_t;
#endif

namespace Detail {
    constexpr char lower_char(char c) { return ((c >= 'A' && c <= 'Z') ? c + 32 : c); }
}

// switch case on a string
constexpr hash_type Hash(const char* input, size_t len) { return (len > 0 ? static_cast<hash_type>(*input) + 33 * Hash(input + 1, len - 1) : 5381); }

template<size_t N>
constexpr hash_type Hash(const char(&input)[N]) { return Hash(input, N-1); }

constexpr hash_type Hash(System::StringView sv) { return Hash(sv.data(), sv.length()); }

inline hash_type Hash(const std::string& input) { return Hash(input.c_str(), input.length()); }



constexpr hash_type IHash(const char* input, size_t len) { return (len > 0 ? static_cast<hash_type>(Detail::lower_char(*input)) + 33 * IHash(input + 1, len - 1) : 5381); }

template<size_t N>
constexpr inline hash_type IHash(const char(&input)[N]) { return IHash(input, N - 1); }

constexpr hash_type IHash(System::StringView sv) { return IHash(sv.data(), sv.length()); }

inline hash_type IHash(const std::string& input) { return IHash(input.c_str(), input.length()); }

}
}
