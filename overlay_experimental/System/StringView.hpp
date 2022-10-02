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

#include <iterator>
#include <string>
#include <iostream>
#include <array>

namespace System {

template<typename char_type>
class BasicStringView {
    const char_type* _string;
    size_t _length;

    using type = BasicStringView<char_type>;

public:
    class iterator {
        const char_type* _value;

    public:
        constexpr iterator(const iterator& o) : _value(o._value) {}
        constexpr iterator(const char_type* value) : _value(value) {}

        constexpr iterator& operator++() { ++_value; return *this; }
        constexpr iterator operator++(int) { iterator retval = *this; ++(*this); return retval; }
        constexpr iterator& operator--() { --_value; return *this; }
        constexpr iterator operator--(int) { iterator retval = *this; --(*this); return retval; }
        constexpr bool operator==(iterator other) const { return _value == other._value; }
        constexpr bool operator!=(iterator other) const { return !(*this == other); }
        constexpr const char_type& operator*() const { return *_value; }

        // iterator traits
        using difference_type = long;
        using value_type = char_type;
        using pointer = const char_type*;
        using reference = const char_type&;
        using iterator_category = std::random_access_iterator_tag;
    };

    constexpr BasicStringView() : _string(nullptr), _length(0)
    {}

    constexpr BasicStringView(const char_type* str, size_t length) : _string(str), _length(length)
    {}

    constexpr BasicStringView(std::basic_string<char_type, std::char_traits<char_type>, std::allocator<char_type>> const& str) : _string(str.data()), _length(str.length())
    {}

    template<size_t N>
    constexpr BasicStringView(const char_type(&str)[N]) : _string(str), _length(N - 1)
    {}

    template<size_t N>
    constexpr BasicStringView(std::array<char_type, N> const& str) : _string(&str.at[0]), _length(N)
    {}

    constexpr BasicStringView(type const& other) : _string(other._string), _length(other._length)
    {}

    constexpr BasicStringView(type&& other) : _string(other._string), _length(other._length)
    {}

    constexpr type& operator=(type const& other)
    {
        _string = other._string;
        _length = other._length;
        return *this;
    }

    constexpr type& operator=(type&& other)
    {
        _string = other._string;
        _length = other._length;
        return *this;
    }

    constexpr const char_type* data() const { return _string; }

    constexpr bool empty() const { return _length == 0; }

    constexpr size_t size() const { return _length; }

    constexpr size_t length() const { return _length; }

    constexpr const char_type& operator[](size_t index) const { return _string[index]; }

    constexpr size_t find_first_of(type const& string, const size_t offset = 0) const
    {
        if(_length == 0)
			return std::string::npos;

        for (size_t i = offset; i < _length; ++i)
        {
            for (size_t j = 0; j < string._length; ++j)
            {
                if (_string[i] == string[j])
                    return i;
            }
        }

        return std::string::npos;
    }

    constexpr size_t find_first_not_of(type const& string, const size_t offset = 0) const
    {
        if(_length == 0)
			return std::string::npos;

        for (size_t i = offset; i < _length; ++i)
        {
            for (size_t j = 0; j < string._length; ++j)
            {
                if (_string[i] == string[j])
                    break;

                if (j == (string._length - 1))
                    return i;
            }
        }

        return std::string::npos;
    }

    constexpr size_t find(type const& string, const size_t offset = 0) const
    {
        if (_length < string._length)
            return std::string::npos;

        for (size_t i = offset; i < (_length - string._length + 1); ++i)
        {
            for (size_t j = 0; j < string._length; ++j)
            {
                if (_string[i + j] != string[j])
                    break;

                if (j == (string._length - 1))
                    return i;
            }
        }

        return std::string::npos;
    }

    constexpr size_t find_not(type const& string, const size_t offset = 0) const
    {
        if (_length < string._length)
            return std::string::npos;

        for (size_t i = offset; i < (_length - string._length + 1); ++i)
        {
            for (size_t j = 0; j < string._length; ++j)
            {
                if (_string[i + j] == string[j])
                    break;

                if (j == (string._length - 1))
                    return i;
            }
        }

        return std::string::npos;
    }

    constexpr size_t count(char_type const c) const
    {
        size_t n = 0;
        for (size_t i = 0; i < _length; ++i)
            if (_string[i] == c)
                ++n;

        return n;
    }

    constexpr type substr(size_t offset, size_t length = std::string::npos) const
    {
        if (offset >= _length)
            return type();

        return type(_string + offset, (_length - offset) > length ? length : _length - offset);
    }

    constexpr iterator begin() const
    {
        return iterator(_string);
    }

    constexpr iterator end() const
    {
        return iterator(_string + _length);
    }

    std::string to_string() const
    {
        return std::string(_string, _string + _length);
    }

    size_t copy(char_type* p, size_t p_size) const
    {
        size_t written = 0;
        if (p != nullptr)
        {
            size_t to_write = (_length > p_size ? p_size : _length);
            char_type* b = _string;
            while (to_write--)
            {
                *p++ = *b++;
                ++written;
            }
        }

        return written;
    }
};

using StringView = BasicStringView<char>;

}

template<typename char_type>
std::basic_ostream<char_type>& operator<<(std::basic_ostream<char_type>& os, System::BasicStringView<char_type> const& sv)
{
    return os.write(sv.data(), sv.length());
}

template<typename char_type>
std::basic_string<char_type> operator+(std::basic_string<char_type> const& str, System::BasicStringView<char_type> const& sv)
{
    std::string r(str);
    return r.append(sv.data(), sv.data() + sv.length());
}

template<typename char_type>
std::basic_string<char_type>& operator+=(std::basic_string<char_type>& str, System::BasicStringView<char_type> const& sv)
{
    return str.append(sv.data(), sv.data() + sv.length());
}