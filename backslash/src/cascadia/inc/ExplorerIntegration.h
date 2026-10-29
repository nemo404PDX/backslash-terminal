// Copyright (c) Backslash contributors. Licensed under the MIT license.
#pragma once
#include <windows.h>
#include <array>
#include <string>
#include <stdexcept>
#include <vector>

namespace Microsoft::Terminal::Backslash::ExplorerIntegration
{
    inline constexpr wchar_t Owner[] = L"Backslash.Explorer.v1.{BF687475-455F-4EA3-AC14-F3FDDF06B825}";
    inline constexpr wchar_t Flag[] = L"--backslash-explorer-folder";
    inline constexpr std::array<const wchar_t*, 2> Surfaces{
        L"Directory\\shell\\Backslash.Open", L"Directory\\Background\\shell\\Backslash.Open"
    };

    inline void Check(LSTATUS code)
    {
        if (code != ERROR_SUCCESS) { throw std::runtime_error("Explorer registry operation failed (Windows error " + std::to_string(code) + "). Refresh status and repair any partial registration."); }
    }

    struct Key
    {
        HKEY value{};
        ~Key() { if (value) { RegCloseKey(value); } }
        Key() = default;
        Key(const Key&) = delete;
        Key& operator=(const Key&) = delete;
    };

    inline std::wstring Read(HKEY key, const wchar_t* name)
    {
        DWORD type{}, bytes{};
        auto result = RegQueryValueExW(key, name, nullptr, &type, nullptr, &bytes);
        if (result == ERROR_FILE_NOT_FOUND) { return {}; }
        Check(result);
        if (type != REG_SZ || bytes < sizeof(wchar_t) || bytes > 131072 || bytes % sizeof(wchar_t)) { throw std::runtime_error("Unexpected registry value type or size; entry left untouched."); }
        std::wstring text(bytes / sizeof(wchar_t), L'\0');
        const auto expectedBytes = bytes;
        Check(RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE*>(text.data()), &bytes));
        if (type != REG_SZ || bytes != expectedBytes) { throw std::runtime_error("Registry value changed while reading; entry left untouched."); }
        if (text.back() != L'\0') { throw std::runtime_error("Invalid registry string; entry left untouched."); }
        text.pop_back();
        if (text.find(L'\0') != std::wstring::npos) { throw std::runtime_error("Invalid registry string; entry left untouched."); }
        return text;
    }

    inline void Write(HKEY key, const wchar_t* name, const std::wstring& text)
    {
        Check(RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(text.c_str()), static_cast<DWORD>((text.size() + 1) * sizeof(wchar_t))));
    }

    inline std::wstring Quote(const std::wstring& text)
    {
        // Windows argv quoting, including a trailing separator before the closing quote.
        std::wstring result = L"\"";
        size_t slashes = 0;
        for (const auto ch : text)
        {
            if (ch == L'\\') { ++slashes; continue; }
            result.append(ch == L'\"' ? slashes * 2 + 1 : slashes, L'\\');
            result += ch;
            slashes = 0;
        }
        result.append(slashes * 2, L'\\');
        return result + L'\"';
    }

    inline std::wstring Command(const std::wstring& exe, size_t surface)
    {
        // The dot prevents Explorer's substituted trailing backslash escaping the quote.
        return Quote(exe) + L" " + Flag + (surface == 0 ? L" \"%1\\.\"" : L" \"%V\\.\"");
    }

    inline bool AbsoluteFolder(const std::wstring& folder)
    {
        // Filesystem drive/UNC only. Reject device namespaces, drive-relative paths and URIs.
        if (folder.size() >= 4 && folder[0] == L'\\' && folder[1] == L'\\' && (folder[2] == L'?' || folder[2] == L'.')) { return false; }
        return (folder.size() >= 3 && ((folder[0] >= L'A' && folder[0] <= L'Z') || (folder[0] >= L'a' && folder[0] <= L'z')) && folder[1] == L':' && folder[2] == L'\\') ||
               (folder.size() > 4 && folder[0] == L'\\' && folder[1] == L'\\' && folder.find(L'\\', 2) != std::wstring::npos);
    }

    inline std::wstring ValidateFolder(const std::wstring& folder)
    {
        if (!AbsoluteFolder(folder) || folder.find(L'\0') != std::wstring::npos || folder.find(L'\"') != std::wstring::npos) { throw std::runtime_error("Open in Backslash requires an absolute filesystem folder (drive or UNC path)."); }
        std::wstring full(32768, L'\0');
        const auto count = GetFullPathNameW(folder.c_str(), static_cast<DWORD>(full.size()), full.data(), nullptr);
        if (!count || count >= full.size()) { throw std::runtime_error("The selected folder path could not be resolved."); }
        full.resize(count);
        const auto attributes = GetFileAttributesW(full.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) { throw std::runtime_error("The selected folder is missing, inaccessible, or is not a filesystem directory."); }
        return full;
    }

    struct Entry { bool exists{}, owned{}, complete{}; std::wstring exe; };

    // The root is injectable for isolated tests. Production uses only HKCU Software\\Classes.
    class Registration
    {
        HKEY _hive;
        std::wstring _root;
        std::wstring Path(size_t index) const { return _root + L"\\" + Surfaces.at(index); }

        static void CheckShape(HKEY key, bool command)
        {
            // A marked key containing foreign data is a collision too. Never delete it recursively.
            for (DWORD i = 0;; ++i)
            {
                wchar_t name[256]; DWORD length = 256;
                const auto result = RegEnumValueW(key, i, name, &length, nullptr, nullptr, nullptr, nullptr);
                if (result == ERROR_NO_MORE_ITEMS) { break; }
                Check(result);
                const std::wstring value(name, length);
                if (!value.empty() && (command || (value != L"BackslashOwner" && value != L"BackslashExecutable" && value != L"Icon" && value != L"MultiSelectModel"))) { throw std::runtime_error("Explorer entry contains unrecognized data; left untouched."); }
                // Validate every present known value before any completeness
                // comparison can short-circuit, including different-copy targets.
                (void)Read(key, value.c_str());
            }
            for (DWORD i = 0;; ++i)
            {
                wchar_t name[256]; DWORD length = 256;
                const auto result = RegEnumKeyExW(key, i, name, &length, nullptr, nullptr, nullptr, nullptr);
                if (result == ERROR_NO_MORE_ITEMS) { break; }
                Check(result);
                if (command || std::wstring(name, length) != L"command") { throw std::runtime_error("Explorer entry contains unrecognized subkeys; left untouched."); }
            }
        }

    public:
        explicit Registration(HKEY hive = HKEY_CURRENT_USER, std::wstring root = L"Software\\Classes") : _hive(hive), _root(std::move(root)) {}
        Entry Inspect(size_t index, const std::wstring& expected) const
        {
            Key key;
            const auto result = RegOpenKeyExW(_hive, Path(index).c_str(), 0, KEY_READ, &key.value);
            if (result == ERROR_FILE_NOT_FOUND) { return {}; }
            Check(result);
            Entry entry; entry.exists = true;
            entry.owned = Read(key.value, L"BackslashOwner") == Owner;
            if (!entry.owned) { return entry; }
            CheckShape(key.value, false);
            entry.exe = Read(key.value, L"BackslashExecutable");
            Key command;
            const auto opened = RegOpenKeyExW(key.value, L"command", 0, KEY_READ, &command.value);
            if (opened == ERROR_FILE_NOT_FOUND) { return entry; }
            Check(opened); CheckShape(command.value, true);
            entry.complete = entry.exe == expected && Read(key.value, L"") == L"Open in Backslash" &&
                Read(key.value, L"Icon") == Quote(expected) + L",0" && Read(key.value, L"MultiSelectModel") == L"Single" && Read(command.value, L"") == Command(expected, index);
            return entry;
        }

        std::array<Entry, 2> Inspect(const std::wstring& exe) const { return { Inspect(0, exe), Inspect(1, exe) }; }

        void Apply(const std::wstring& exe, bool remove) const
        {
            // Preflight both surfaces before touching either. Recheck immediately before each mutation.
            const auto entries = Inspect(exe);
            for (const auto& entry : entries) { if (entry.exists && !entry.owned) { throw std::runtime_error("Backslash.Open is owned by another registration or has no ownership marker. Nothing changed; resolve the collision manually."); } }
            for (size_t i = 0; i < Surfaces.size(); ++i)
            {
                const auto current = Inspect(i, exe);
                if (current.exists && !current.owned) { throw std::runtime_error("Registration changed during the operation; stopped."); }
                if (remove)
                {
                    if (!current.exists) { continue; }
                    Key key; Check(RegOpenKeyExW(_hive, Path(i).c_str(), 0, KEY_READ | KEY_WRITE, &key.value));
                    const auto deleted = RegDeleteKeyW(key.value, L"command");
                    if (deleted != ERROR_FILE_NOT_FOUND) { Check(deleted); }
                    Check(RegDeleteKeyW(_hive, Path(i).c_str()));
                }
                else
                {
                    Key key; DWORD disposition{};
                    Check(RegCreateKeyExW(_hive, Path(i).c_str(), 0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &key.value, &disposition));
                    if (disposition != REG_CREATED_NEW_KEY && Read(key.value, L"BackslashOwner") != Owner) { throw std::runtime_error("Registration changed during the operation; stopped."); }
                    Write(key.value, L"BackslashOwner", Owner);
                    Write(key.value, L"BackslashExecutable", exe);
                    Write(key.value, L"", L"Open in Backslash");
                    Write(key.value, L"Icon", Quote(exe) + L",0");
                    Write(key.value, L"MultiSelectModel", L"Single");
                    Key command; Check(RegCreateKeyExW(key.value, L"command", 0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &command.value, nullptr));
                    Write(command.value, L"", Command(exe, i));
                }
            }
        }
    };
}
