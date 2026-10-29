// Copyright (c) Backslash contributors. Licensed under the MIT license.
#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
#include <cwctype>
#include "../../../src/cascadia/inc/ExplorerIntegration.h"

namespace Backslash::ModernExplorer
{
    namespace Legacy = Microsoft::Terminal::Backslash::ExplorerIntegration;
    inline constexpr wchar_t Owner[] = L"Backslash.ExplorerIntegration.Modern.v1";
    inline constexpr wchar_t ProductionKey[] = L"Software\\Backslash\\ExplorerIntegration";
    inline constexpr wchar_t PackageName[] = L"Backslash.ExplorerIntegration.Dev";
    inline constexpr wchar_t Publisher[] = L"CN=Backslash Development";
    struct Error : std::runtime_error
    {
        HRESULT code;
        Error(HRESULT value, const char* message) : std::runtime_error(message), code(value) {}
    };
    inline void Check(LSTATUS code) { if (code != ERROR_SUCCESS) { throw Error(HRESULT_FROM_WIN32(code), "Registry operation failed"); } }
    inline bool EqualPath(const std::wstring& a, const std::wstring& b) { return CompareStringOrdinal(a.c_str(), -1, b.c_str(), -1, TRUE) == CSTR_EQUAL; }
    inline std::wstring FullPath(const std::wstring& path)
    {
        if (!Legacy::AbsoluteFolder(path) || path.find(L'\0') != std::wstring::npos || path.find(L'"') != std::wstring::npos) { throw Error(E_INVALIDARG, "An absolute drive or UNC filesystem path is required; device paths are unsupported"); }
        std::wstring result(32768, L'\0');
        const auto count = GetFullPathNameW(path.c_str(), static_cast<DWORD>(result.size()), result.data(), nullptr);
        if (!count || count >= result.size()) { throw Error(E_INVALIDARG, "Path could not be resolved"); }
        result.resize(count); return result;
    }
    inline std::wstring ValidateTarget(const std::wstring& path)
    {
        auto result = FullPath(path);
        if (!EqualPath(std::filesystem::path(result).filename().wstring(), L"Backslash.exe")) { throw Error(E_INVALIDARG, "Select Backslash.exe from a compatible portable build"); }
        DWORD binaryType{};
        if (!GetBinaryTypeW(result.c_str(), &binaryType) || binaryType != SCS_64BIT_BINARY) { throw Error(E_INVALIDARG, "Target is missing or is not an x64 Windows executable"); }
        const auto file = CreateFileW(result.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) { throw Error(HRESULT_FROM_WIN32(GetLastError()), "Cannot read target executable"); }
        struct Close { HANDLE handle; ~Close() { CloseHandle(handle); } } close{file};
        IMAGE_DOS_HEADER dos{}; DWORD read{};
        if (!ReadFile(file, &dos, sizeof(dos), &read, nullptr) || read != sizeof(dos) || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0 || dos.e_lfanew > 32 * 1024 * 1024) { throw Error(E_INVALIDARG, "Invalid target executable header"); }
        LARGE_INTEGER offset{}; offset.QuadPart = dos.e_lfanew; DWORD signature{}; IMAGE_FILE_HEADER header{};
        if (!SetFilePointerEx(file, offset, nullptr, FILE_BEGIN) || !ReadFile(file, &signature, sizeof(signature), &read, nullptr) || read != sizeof(signature) || signature != IMAGE_NT_SIGNATURE || !ReadFile(file, &header, sizeof(header), &read, nullptr) || read != sizeof(header) || header.Machine != IMAGE_FILE_MACHINE_AMD64 || (header.Characteristics & IMAGE_FILE_DLL)) { throw Error(E_INVALIDARG, "Target must be an x64 executable, not another architecture or a DLL"); }
        const auto component = std::filesystem::path(result).parent_path() / L"TerminalApp.dll";
        const auto attributes = GetFileAttributesW(component.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) { throw Error(E_INVALIDARG, "Target folder is incomplete: TerminalApp.dll is missing"); }
        return result;
    }
    inline std::wstring LaunchCommand(const std::wstring& exe, const std::wstring& folder)
    {
        return Legacy::Quote(exe) + L" " + Legacy::Flag + L" " + Legacy::Quote(folder);
    }
    struct Configuration { bool exists{}; std::wstring target, support; };
    class TargetStore
    {
        HKEY _hive; std::wstring _path;
    public:
        explicit TargetStore(HKEY hive = HKEY_CURRENT_USER, std::wstring path = ProductionKey) : _hive(hive), _path(std::move(path)) {}
        Configuration Read() const
        {
            Legacy::Key key;
            const auto opened = RegOpenKeyExW(_hive, _path.c_str(), 0, KEY_READ, &key.value);
            if (opened == ERROR_FILE_NOT_FOUND) { return {}; } Check(opened);
            for (DWORD i = 0;; ++i)
            {
                wchar_t name[256]; DWORD length = 256;
                auto code = RegEnumValueW(key.value, i, name, &length, nullptr, nullptr, nullptr, nullptr);
                if (code == ERROR_NO_MORE_ITEMS) { break; } Check(code);
                const std::wstring value(name, length);
                if (value != L"Owner" && value != L"TargetExecutable" && value != L"SupportRoot" && value != L"SchemaVersion") { throw Error(E_ACCESSDENIED, "Unrecognized target configuration value; untouched"); }
                (void)Legacy::Read(key.value, name); // Validate every present kind/byte sequence, not just the owner.
            }
            DWORD children{}; Check(RegQueryInfoKeyW(key.value, nullptr, nullptr, nullptr, &children, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));
            if (children || Legacy::Read(key.value, L"Owner") != Owner) { throw Error(E_ACCESSDENIED, "Foreign or unmarked target configuration; untouched"); }
            const auto version = Legacy::Read(key.value, L"SchemaVersion");
            if (!version.empty() && version != L"1") { throw Error(E_ACCESSDENIED, "Unsupported target configuration version; untouched"); }
            return { true, Legacy::Read(key.value, L"TargetExecutable"), Legacy::Read(key.value, L"SupportRoot") };
        }
        void Set(const std::wstring& target, const std::wstring& support, const std::wstring& expected) const
        {
            const auto before = Read();
            if (!EqualPath(before.target, expected)) { throw Error(HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH), "Target changed; inspect status and confirm again"); }
            const auto validated = ValidateTarget(target); const auto root = Legacy::ValidateFolder(support);
            Legacy::Key key; DWORD disposition{};
            Check(RegCreateKeyExW(_hive, _path.c_str(), 0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &key.value, &disposition));
            if (disposition != REG_CREATED_NEW_KEY)
            {
                const auto current = Read();
                if (!EqualPath(current.target, expected)) { throw Error(HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH), "Target changed; operation stopped"); }
            }
            Legacy::Write(key.value, L"Owner", Owner); Legacy::Write(key.value, L"SchemaVersion", L"1");
            Legacy::Write(key.value, L"SupportRoot", root); Legacy::Write(key.value, L"TargetExecutable", validated);
        }
        void Remove(const std::wstring& expected) const
        {
            const auto current = Read(); if (!current.exists) { if (!expected.empty()) { throw Error(E_INVALIDARG, "Target no longer exists; inspect status"); } return; }
            if (!EqualPath(current.target, expected)) { throw Error(HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH), "A different portable copy is selected; confirm its exact path first"); }
            Check(RegDeleteKeyW(_hive, _path.c_str()));
        }
    };
    struct LaunchRequest { std::wstring executable, command, folder; };
    inline LaunchRequest Prepare(const TargetStore& store, const std::wstring& folder)
    {
        const auto config = store.Read();
        if (!config.exists || config.target.empty()) { throw Error(HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "No portable target is selected. Use the integration host to select a Backslash copy"); }
        const auto exe = ValidateTarget(config.target); const auto directory = Legacy::ValidateFolder(folder);
        return { exe, LaunchCommand(exe, directory), directory };
    }
    inline std::wstring Json(const std::wstring& value)
    {
        std::wstring out = L"\"";
        for (const auto ch : value)
        {
            if (ch == L'"' || ch == L'\\') { out += L'\\'; out += ch; }
            else if (ch < 32 || ch > 126) { wchar_t encoded[7]; swprintf_s(encoded, L"\\u%04x", static_cast<unsigned>(ch)); out += encoded; }
            else { out += ch; }
        }
        return out + L'"';
    }
}
