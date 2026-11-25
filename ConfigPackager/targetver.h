#pragma once

// Minimum required platform. Include this FIRST, before any header that pulls in <windows.h>, so the
// Windows SDK doesn't emit "_WIN32_WINNT not defined. Defaulting to _WIN32_WINNT_MAXVER" and silently
// pick a default on its own. _WIN32_WINNT_MAXVER tracks the newest version the installed SDK knows about
// (it has been 0x0A00, Windows 10, for the entire Windows 10/11 SDK era).

#include <WinSDKVer.h>
#define _WIN32_WINNT _WIN32_WINNT_MAXVER
#include <SDKDDKVer.h>
