#pragma once

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  // Toolhelp requires Windows types; preserve this include order.
  // clang-format off
  #include <Windows.h>
  #include <TlHelp32.h>
  // clang-format on
  #include <psapi.h>
#else
  #include <sys/resource.h>
  #include <unistd.h>
#endif

// Headers containing OS declarations must precede import std.
