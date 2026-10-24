// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <string_view>

// Dev is the local Backslash product. Keep upstream channel filenames unchanged;
// this is a file/display-name change, not a package identity or alias migration.
#if defined(WT_BRANDING_RELEASE) || defined(WT_BRANDING_PREVIEW) || defined(WT_BRANDING_CANARY)
inline constexpr std::wstring_view TerminalExe{ L"WindowsTerminal.exe" };
inline constexpr std::wstring_view TerminalDisplayName{ L"Windows Terminal" };
#else
inline constexpr std::wstring_view TerminalExe{ L"Backslash.exe" };
inline constexpr std::wstring_view TerminalDisplayName{ L"Backslash" };
#endif
