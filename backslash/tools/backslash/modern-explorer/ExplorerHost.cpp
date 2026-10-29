// Copyright (c) Backslash contributors. Licensed under the MIT license.
#include "TargetStore.h"
#include "ManifestIdentity.h"
#include <iostream>
#include <appmodel.h>
using namespace Backslash::ModernExplorer;
int wmain(int argc, wchar_t** argv)
{
    try
    {
        if (argc == 3 && std::wstring(argv[1]) == L"--manifest-identity") { std::wcout << IdentityJson(argv[2]) << L'\n'; return 0; }
        TargetStore store;
        // Viewing status is always read-only; no package registration is implemented here.
        if (argc == 1 || (argc == 2 && std::wstring(argv[1]) == L"--status"))
        {
            const auto current = store.Read();
            UINT32 length{}; const auto identity = GetCurrentPackageFullName(&length, nullptr);
            std::wcout << L"{\"exists\":" << (current.exists ? L"true" : L"false") << L",\"target\":" << Json(current.target) << L",\"support\":" << Json(current.support) << L",\"hostHasPackageIdentity\":" << (identity == APPMODEL_ERROR_NO_PACKAGE ? L"false" : L"true") << L"}\n"; return 0;
        }
        std::vector<wchar_t> module(32768); const auto length = GetModuleFileNameW(nullptr, module.data(), static_cast<DWORD>(module.size()));
        if (!length || length >= module.size()) { throw Error(E_FAIL, "Cannot identify integration host folder"); }
        const auto support = std::filesystem::path(std::wstring(module.data(), length)).parent_path().wstring();
        if (argc == 7 && std::wstring(argv[1]) == L"--use-copy" && std::wstring(argv[3]) == L"--expected-current" && std::wstring(argv[5]) == L"--apply" && std::wstring(argv[6]) == L"target-only")
        { store.Set(argv[2], support, std::wstring(argv[4]) == L"<none>" ? L"" : argv[4]); std::wcout << L"Target selected. Package deployment was not performed.\n"; return 0; }
        if (argc == 6 && std::wstring(argv[1]) == L"--remove-target" && std::wstring(argv[2]) == L"--expected-current" && std::wstring(argv[4]) == L"--apply" && std::wstring(argv[5]) == L"target-only")
        { store.Remove(std::wstring(argv[3]) == L"<none>" ? L"" : argv[3]); std::wcout << L"Owned target configuration removed. Package removal was not performed.\n"; return 0; }
        std::wcerr << L"Usage: --status | --use-copy <Backslash.exe> --expected-current <path or <none>> --apply target-only | --remove-target --expected-current <path or <none>> --apply target-only\nThis prototype never deploys packages or edits classic Explorer verbs.\n";
        return 2;
    }
    catch (const Error& error) { std::cerr << "FAILED: " << error.what() << " HRESULT=" << std::hex << static_cast<unsigned>(error.code) << '\n'; return 1; }
    catch (const std::exception& error) { std::cerr << "FAILED: " << error.what() << '\n'; return 1; }
}
