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

#include <cstddef> // size_t
#include <cstdint> // uint*_t

namespace System {
	class Endian
    {
    private:
        template<typename T, size_t byte_count>
        struct ByteSwapImpl
        {
            constexpr static inline T swap(T v)
            {
                for (int i = 0; i < (byte_count/2); ++i)
                {
                    uint8_t tmp = reinterpret_cast<uint8_t*>(&v)[i];
                    reinterpret_cast<uint8_t*>(&v)[i] = reinterpret_cast<uint8_t*>(&v)[byte_count - i - 1];
                    reinterpret_cast<uint8_t*>(&v)[byte_count - i - 1] = tmp;
                }

                return v;
            }
        };

        template<typename T>
        struct ByteSwapImpl<T, 1>
        {
            constexpr static inline T swap(T v) { return v; }
        };

        template<typename T>
        struct ByteSwapImpl<T, 2>
        {
            constexpr static inline T swap(T v)
            {
                uint16_t& tmp = *reinterpret_cast<uint16_t*>(&v);
                tmp = ((tmp & 0x00ffu) << 8)
                    | ((tmp & 0xff00u) >> 8);
                return v;
            }
        };

        template<typename T>
        struct ByteSwapImpl<T, 4>
        {
            constexpr static inline T swap(T v)
            {
                uint32_t& tmp = *reinterpret_cast<uint32_t*>(&v);
                tmp = ((tmp & 0x000000fful) << 24)
                    | ((tmp & 0x0000ff00ul) << 8)
                    | ((tmp & 0x00ff0000ul) >> 8)
                    | ((tmp & 0xff000000ul) >> 24);
                return v;
            }
        };

        template<typename T>
        struct ByteSwapImpl<T, 8>
        {
            constexpr static inline T swap(T v)
            {
                uint64_t& tmp = *reinterpret_cast<uint64_t*>(&v);
                tmp = ((tmp & 0x00000000000000ffull) << 56)
                    | ((tmp & 0x000000000000ff00ull) << 40)
                    | ((tmp & 0x0000000000ff0000ull) << 24)
                    | ((tmp & 0x00000000ff000000ull) << 8)
                    | ((tmp & 0x000000ff00000000ull) >> 8)
                    | ((tmp & 0x0000ff0000000000ull) >> 24)
                    | ((tmp & 0x00ff000000000000ull) >> 40)
                    | ((tmp & 0xff00000000000000ull) >> 56);
                return v;
            }
        };

    public:
        static inline bool little()
        {
            const uint32_t endian_magic = 0x01020304;
            return reinterpret_cast<const uint8_t*>(&endian_magic)[0] == 0x04;
        }

        static inline bool big()
        {
            const uint32_t endian_magic = 0x01020304;
            return reinterpret_cast<const uint8_t*>(&endian_magic)[0] == 0x01;
        }

        template<typename T, size_t Size = sizeof(T)>
        constexpr static inline T net_swap(T v)
        {
            if(Endian::little())
            {
                return ByteSwapImpl<T, Size>::swap(v);
            }

            return v;
        }

        template<typename T, size_t Size = sizeof(T)>
        constexpr static inline T swap(T v)
        {
            return ByteSwapImpl<T, Size>::swap(v);
        }
    
    private:
        Endian() = delete;
    };
}