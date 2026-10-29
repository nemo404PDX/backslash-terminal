// Copyright (c) Backslash contributors. Licensed under the MIT license.
// Isolated test child only. Never a deployed package or terminal replacement.
#include "TargetStore.h"
#include <appmodel.h>
#include <fstream>
using namespace Backslash::ModernExplorer;
int wmain(int argc, wchar_t** argv)
{
    if (argc != 3 || std::wstring(argv[1]) != Legacy::Flag) { return 2; }
    std::wstring cwd(32768, L'\0'); const auto count = GetCurrentDirectoryW(static_cast<DWORD>(cwd.size()), cwd.data()); if (!count || count >= cwd.size()) { return 3; } cwd.resize(count);
    UINT32 length{}; const auto identity = GetCurrentPackageFullName(&length, nullptr);
    const auto json = L"{\"argument\":" + Json(argv[2]) + L",\"cwd\":" + Json(cwd) + L",\"identity\":" + Json(identity == APPMODEL_ERROR_NO_PACKAGE ? L"unpackaged" : L"packaged") + L"}";
    const auto temporary = std::filesystem::path(argv[2]) / L"modern-launch-observed.tmp"; const auto final = std::filesystem::path(argv[2]) / L"modern-launch-observed.json";
    { std::ofstream output(temporary); for (const auto ch : json) { output.put(static_cast<char>(ch)); } if (!output.good()) { return 4; } }
    std::filesystem::rename(temporary, final); return 0;
}
