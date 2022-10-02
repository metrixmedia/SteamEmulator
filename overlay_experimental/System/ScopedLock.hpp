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

#include <tuple>
#include <mutex>

namespace System {

class scoped_lock {
    struct value_holder
    {
        virtual ~value_holder() noexcept {}
    };

    template<typename... Args>
    struct templated_value_holder : value_holder
    {
        template<std::size_t I = 0, typename... Tp>
        inline typename std::enable_if<I == sizeof...(Tp), void>::type unlock(std::tuple<Tp...>& t) { }

        template<std::size_t I = 0, typename... Tp>
        inline typename std::enable_if<I < sizeof...(Tp), void>::type unlock(std::tuple<Tp...>& t)
        {
            std::get<I>(t).unlock();
            unlock<I + 1, Tp...>(t);
        }

        explicit templated_value_holder(Args&... mutexes) : _mutexes(mutexes...) { std::lock(mutexes...); }
        explicit templated_value_holder(std::adopt_lock_t, Args&... mutexes) : _mutexes(mutexes...) {} // construct but don't lock

        virtual ~templated_value_holder() noexcept { unlock(_mutexes); }

        std::tuple<Args&...> _mutexes;
    };

    template<typename Arg>
    struct templated_value_holder<Arg> : value_holder
    {
        explicit templated_value_holder(Arg& mutex) : _mutex(mutex) { _mutex.lock(); }
        explicit templated_value_holder(std::adopt_lock_t, Arg& mutex) : _mutex(mutex) {} // construct but don't lock

        virtual ~templated_value_holder() noexcept { _mutex.unlock(); }

        Arg& _mutex;
    };

    value_holder* _val;

public:
    template<typename... Args>
    explicit scoped_lock(Args&... mutexes) : _val(new templated_value_holder<Args&...>(mutexes...)) { }

    template<typename... Args>
    explicit scoped_lock(std::adopt_lock_t, Args&... mutexes) : _val(new templated_value_holder<Args&...>(std::adopt_lock, mutexes...)) { }

    explicit scoped_lock(scoped_lock && other):
        _val(other._val)
    {
        other._val = nullptr;
    }

    scoped_lock() noexcept:
        _val(nullptr)
    {}
    ~scoped_lock() noexcept { delete _val; }
    
    scoped_lock& operator=(scoped_lock && other)
    {
        _val = other._val;
        other._val = nullptr;
        return *this;
    }

    scoped_lock(const scoped_lock&) = delete;
    scoped_lock& operator=(const scoped_lock&) = delete;
};

}