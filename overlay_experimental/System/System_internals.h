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

#include "SystemExports.h"

#if defined(SYSTEM_OS_WINDOWS)

#include <string>

namespace System {
}

#elif defined(SYSTEM_OS_LINUX)

#include <string>

namespace System {
SYSTEM_HIDE_API(std::string , SYSTEM_CALL_DEFAULT) ExpandSymlink(std::string file_path);
}

#endif
