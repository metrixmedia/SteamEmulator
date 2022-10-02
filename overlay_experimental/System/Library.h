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

#include <string>

namespace System {

namespace Library {

class Library
{
    class LibraryImpl* _Impl;

public:
    Library();

    Library(Library const& other);

    Library(Library&& other) noexcept;

    Library& operator=(Library const& other);

    Library& operator=(Library&& other) noexcept;

    ~Library();

    bool OpenLibrary(std::string const& library_name, bool append_extension);

    void CloseLibrary();

    void* GetVSymbol(std::string const& symbol_name) const;

    template<typename T>
    inline T* GetSymbol(std::string const& symbol_name) const
    {
        return reinterpret_cast<T*>(GetVSymbol(symbol_name));
    }

    std::string GetLibraryPath() const;

    void* GetLibraryNativeHandle() const;

    bool IsLoaded() const;
};

// Triies to load the library, I suggest that you use a Library instance instead
void* OpenLibrary(const char* library_name);
// Will decrease the OS' ref counter on the library, use it to close a handle opened by open_library.
// A Library instance will automatically call this in the destructor
void  CloseLibrary(void* handle);
// Will try to retrieve a symbol address from the library handle
void* GetSymbol(void* handle, const char* symbol_name);
// Get a pointer to the library, if it is not loaded, will return nullptr. This doesn't increment the OS' internal ref counter
void* GetLibraryHandle(const char* library_name);
// Get the library path of a module handle
std::string GetLibraryPath(void* handle);
// Get the native extension representing a shared library.
std::string GetLibraryExtension();

}

}
