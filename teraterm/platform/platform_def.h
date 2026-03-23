/*
 * Copyright (C) 2024 TeraTerm Project
 * All rights reserved.
 *
 * Platform abstraction layer - Core type definitions
 *
 * This header provides Windows-compatible type definitions for non-Windows
 * platforms (macOS, Linux), enabling Tera Term source code to compile with
 * minimal changes.
 */
#pragma once

#if defined(_WIN32) || defined(_WIN64)
  /* On Windows, use native headers */
  #define TT_PLATFORM_WINDOWS 1
  #include <windows.h>
#elif defined(__APPLE__)
  #define TT_PLATFORM_MACOS 1
  #include "platform_macos_types.h"
#elif defined(__linux__)
  #define TT_PLATFORM_LINUX 1
  #include "platform_macos_types.h"  /* Reuse POSIX types */
#else
  #error "Unsupported platform"
#endif
