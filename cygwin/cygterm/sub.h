/*
 * Copyright (C) 2022- TeraTerm Project
 * All rights reserved.
 *
 * This file is part of CygTerm+
 *
 * CygTerm+ is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (GPL) as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * CygTerm+ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cygterm; if not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

BOOL IsPortableMode(void);
char *GetAppDataDirU8(void);
wchar_t *ToWcharU8(const char *strU8);
char *ToU8W(const wchar_t *strW);
char *GetModuleFileNameU8(void);
