// Copyright (c) Backslash contributors. Licensed under the MIT license.
#include "../../src/cascadia/inc/ExplorerIntegration.h"
#include <shellapi.h>
#include <filesystem>
#include <iostream>
using namespace Microsoft::Terminal::Backslash::ExplorerIntegration;
void Require(bool value) { if (!value) { throw std::runtime_error("Assertion failed"); } }
template<typename F> void Reject(F action) { bool rejected = false; try { action(); } catch (const std::exception&) { rejected = true; } Require(rejected); }
std::string Snapshot(HKEY key)
{
    std::string result;
    for (DWORD i = 0;; ++i)
    {
        wchar_t name[256]; DWORD length = 256, type{}, bytes{};
        const auto code = RegEnumValueW(key, i, name, &length, nullptr, &type, nullptr, &bytes);
        if (code == ERROR_NO_MORE_ITEMS) { break; }
        Check(code);
        std::vector<BYTE> data(bytes); Check(RegQueryValueExW(key, name, nullptr, &type, data.data(), &bytes));
        result.append(reinterpret_cast<const char*>(name), length * sizeof(wchar_t));
        result += ':' + std::to_string(type) + ':' + std::to_string(bytes) + ':';
        result.append(reinterpret_cast<const char*>(data.data()), data.size());
    }
    for (DWORD i = 0;; ++i)
    {
        wchar_t name[256]; DWORD length = 256;
        const auto code = RegEnumKeyExW(key, i, name, &length, nullptr, nullptr, nullptr, nullptr);
        if (code == ERROR_NO_MORE_ITEMS) { break; }
        Check(code); result.append(reinterpret_cast<const char*>(name), length * sizeof(wchar_t));
        Key child; Check(RegOpenKeyExW(key, name, 0, KEY_READ, &child.value)); result += '{' + Snapshot(child.value) + '}';
    }
    return result;
}
void MalformedMatrix(Registration& registry, const std::wstring& root, const std::wstring& exe, const std::wstring& other)
{
    const std::wstring payload = L"bad";
    struct Bad { DWORD type; std::vector<BYTE> data; };
    const auto bytes = [](const std::wstring& text, bool terminator) {
        const auto begin = reinterpret_cast<const BYTE*>(text.c_str());
        return std::vector<BYTE>(begin, begin + (text.size() + (terminator ? 1 : 0)) * sizeof(wchar_t));
    };
    const std::vector<Bad> bad{
        { REG_EXPAND_SZ, bytes(Owner, true) }, { REG_DWORD, {1,0,0,0} }, { REG_BINARY, {1,2} },
        { REG_MULTI_SZ, {0,0,0,0} }, { REG_SZ, {} }, { REG_SZ, {1} },
        { REG_SZ, bytes(payload, false) }, { REG_SZ, bytes(std::wstring{L'a', L'\0', L'b'}, true) }
    };
    size_t cases = 0;
    for (size_t surface = 0; surface < Surfaces.size(); ++surface)
    {
        for (const auto name : { L"BackslashOwner", L"BackslashExecutable", L"", L"Icon", L"MultiSelectModel", L"command" })
        {
            for (const auto& malformed : bad)
            {
                registry.Apply(other, false);
                Key target;
                const auto path = root + L"\\" + Surfaces[surface] + (std::wstring{name} == L"command" ? L"\\command" : L"");
                Check(RegOpenKeyExW(HKEY_CURRENT_USER, path.c_str(), 0, KEY_READ | KEY_WRITE, &target.value));
                const auto valueName = std::wstring{name} == L"command" ? L"" : name;
                Check(RegSetValueExW(target.value, valueName, 0, malformed.type, malformed.data.data(), static_cast<DWORD>(malformed.data.size())));
                Key tree; Check(RegOpenKeyExW(HKEY_CURRENT_USER, root.c_str(), 0, KEY_READ, &tree.value));
                const auto before = Snapshot(tree.value);
                Reject([&] { registry.Inspect(exe); }); // expected differs: no comparison may skip validation
                Reject([&] { registry.Apply(exe, false); }); Require(Snapshot(tree.value) == before);
                Reject([&] { registry.Apply(exe, true); }); Require(Snapshot(tree.value) == before);
                // Restore only this test's exact value so the next matrix case starts valid.
                std::wstring original = std::wstring{name} == L"BackslashOwner" ? Owner : std::wstring{name} == L"BackslashExecutable" ? other :
                    std::wstring{name} == L"Icon" ? Quote(other) + L",0" : std::wstring{name} == L"MultiSelectModel" ? L"Single" :
                    std::wstring{name} == L"command" ? Command(other, surface) : L"Open in Backslash";
                Write(target.value, valueName, original);
                ++cases;
            }
        }
    }
    registry.Apply(exe, true);
    std::wcout << L"PASS: " << cases << L" malformed known-value cases; both surfaces byte-preserved for failed repoint/remove preflight\n";
}
int wmain(int argc, wchar_t** argv)
{
    try
    {
        Require(argc == 2);
        const std::wstring root = L"Software\\Backslash.ExplorerIntegration.Tests\\" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
        Registration registry{ HKEY_CURRENT_USER, root };
        // This process creates and removes ONLY its unique synthetic registry tree.
        struct Cleanup { std::wstring root; ~Cleanup() { RegDeleteTreeW(HKEY_CURRENT_USER, root.c_str()); } } cleanup{root};
        const std::wstring exe = L"C:\\Dev & tools\\日本語\\Backslash.exe";
        const std::wstring other = L"D:\\Old Copy\\Backslash.exe";
        Require(!registry.Inspect(exe)[0].exists);
        registry.Apply(exe, false); Require(registry.Inspect(exe)[0].complete && registry.Inspect(exe)[1].complete);
        registry.Apply(exe, false); Require(registry.Inspect(exe)[0].complete);
        registry.Apply(other, false); Require(registry.Inspect(other)[0].complete && !registry.Inspect(exe)[0].complete);
        // Partial/corrupt owned command is recoverable; a different copy never changes merely on inspection.
        const auto path = root + L"\\" + Surfaces[0];
        Key key; Check(RegOpenKeyExW(HKEY_CURRENT_USER, path.c_str(), 0, KEY_ALL_ACCESS, &key.value));
        Check(RegDeleteKeyW(key.value, L"command")); Require(!registry.Inspect(other)[0].complete);
        registry.Apply(exe, false); Require(registry.Inspect(exe)[0].complete);
        Write(key.value, L"ForeignData", L"preserve");
        Reject([&] { registry.Apply(exe, true); }); Require(registry.Inspect(1, exe).complete);
        Check(RegDeleteValueW(key.value, L"ForeignData"));
        Write(key.value, L"BackslashOwner", L"foreign");
        Reject([&] { registry.Apply(exe, false); }); Reject([&] { registry.Apply(exe, true); });
        Check(RegDeleteValueW(key.value, L"BackslashOwner")); Reject([&] { registry.Apply(exe, false); });
        Write(key.value, L"BackslashOwner", Owner);
        Key foreign; Check(RegCreateKeyExW(key.value, L"unknown-child", 0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &foreign.value, nullptr));
        Reject([&] { registry.Apply(exe, true); });
        Check(RegDeleteKeyW(key.value, L"unknown-child"));
        registry.Apply(exe, true); Require(!registry.Inspect(exe)[0].exists && !registry.Inspect(exe)[1].exists);
        registry.Apply(exe, true);
        MalformedMatrix(registry, root, exe, other);
        std::wcout << L"PASS: enable, idempotency, repoint, partial repair, bounded removal, foreign/unmarked/unknown-data refusal\n";
        for (const std::wstring folder : { L"C:\\", L"C:\\space folder\\", L"C:\\日本語 & % ! ^ (x);new-tab\\;x", L"\\\\server\\share\\", L"C:\\a\\\\" })
        {
            // Exercise Explorer substitution AFTER command construction with real Windows argv parsing.
            for (size_t surface = 0; surface < Surfaces.size(); ++surface)
            {
                auto command = Command(exe, surface);
                const auto placeholder = surface == 0 ? L"%1" : L"%V";
                command.replace(command.find(placeholder), 2, folder);
                int count{}; auto parsed = CommandLineToArgvW(command.c_str(), &count);
                Require(parsed && count == 3); Require(parsed[0] == exe); Require(std::wstring(parsed[1]) == Flag); Require(std::wstring(parsed[2]) == folder + L"\\."); LocalFree(parsed);
            }
        }
        std::wcout << L"PASS: direct command argv boundaries, Unicode, spaces, roots, trailing separators, UNC, metacharacters, semicolons\n";
        const auto fixture = std::filesystem::path(argv[1]) / L"日本語 & ;folder";
        std::filesystem::create_directory(fixture);
        Require(ValidateFolder(fixture.wstring() + L"\\.") == fixture.wstring());
        Require(!ValidateFolder(fixture.root_path().wstring()).empty());
        Reject([&] { ValidateFolder((fixture / L"missing").wstring()); });
        Reject([&] { ValidateFolder((std::filesystem::path(argv[1]) / L"ExplorerIntegrationTests.exe").wstring()); });
        for (const auto invalid : { L"C:relative", L"relative", L"shell:Downloads", L"\\\\?\\C:\\", L"\\\\.\\pipe\\x", L"\\rootrelative" }) { Reject([&] { ValidateFolder(invalid); }); }
        std::wcout << L"PASS: folder validation, existing filesystem/root, nonexistent and unsupported rejection\n";
        return 0;
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
