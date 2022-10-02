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

#include <chrono>
#include <string>
#include <vector>

namespace System {

std::chrono::system_clock::time_point GetBootTime();
std::chrono::microseconds GetUpTime();

// Get the current process argv
std::vector<std::string> GetProcArgs();
// Get User env variable
std::string GetEnvVar(std::string const& var);
// User appdata full path
std::string GetUserdataPath();
// Executable full path
std::string GetExecutablePath();
// .dll, .so or .dylib full path
std::string GetModulePath();
// List all loaded modules
std::vector<std::string> GetModules();

}
