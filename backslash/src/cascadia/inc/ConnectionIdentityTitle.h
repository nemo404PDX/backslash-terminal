// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <string_view>
#include <algorithm>
#include <iterator>
#include <cwctype>
#include <wil/resource.h>

namespace Microsoft::Terminal::Backslash::ConnectionIdentityTitle
{
    enum class LaunchKind { LocalShell, Ssh, Wsl, Unknown };

    // This only classifies a direct launcher; it never treats parsed arguments
    // as an authenticated host/distro identity or follows arbitrary wrappers.
    inline LaunchKind Classify(std::wstring_view commandline)
    {
        if (commandline.empty() || commandline.find_first_of(std::wstring_view{ L"\0\r\n", 3 }) != commandline.npos) { return LaunchKind::Unknown; }
        const std::wstring copy{ commandline };
        int count = 0;
        const wil::unique_hlocal_ptr<wchar_t*[]> args{ CommandLineToArgvW(copy.c_str(), &count) };
        if (!args || count < 1) { return LaunchKind::Unknown; }
        std::wstring exe{ args[0] };
        if (const auto pos = exe.find_last_of(L"\\/"); pos != exe.npos) { exe.erase(0, pos + 1); }
        std::transform(exe.begin(), exe.end(), exe.begin(), [](wchar_t ch) { return static_cast<wchar_t>(towlower(ch)); });
        if (exe == L"ssh" || exe == L"ssh.exe") { return LaunchKind::Ssh; }
        if (exe == L"wsl" || exe == L"wsl.exe") { return LaunchKind::Wsl; }
        if (exe != L"cmd" && exe != L"cmd.exe" && exe != L"pwsh" && exe != L"pwsh.exe" && exe != L"powershell" && exe != L"powershell.exe") { return LaunchKind::Unknown; }
        for (int i = 1; i < count; ++i)
        {
            std::wstring flag{ args[i] };
            std::transform(flag.begin(), flag.end(), flag.begin(), [](wchar_t ch) { return static_cast<wchar_t>(towlower(ch)); });
            // Allow only normal local-shell flags. Abbreviated/colon remote
            // flags, scripts, commands and wrappers remain conservatively unknown.
            const bool cmd = exe == L"cmd" || exe == L"cmd.exe";
            const bool plain = cmd ? (flag == L"/d" || flag == L"/q" || flag == L"/a" || flag == L"/u") :
                (flag == L"-nologo" || flag == L"-noprofile" || flag == L"-noexit" || flag == L"-interactive" || flag == L"-noninteractive" || flag == L"-login" || flag == L"-l" || flag == L"-sta" || flag == L"-mta");
            if (!plain) { return LaunchKind::Unknown; }
        }
        return LaunchKind::LocalShell;
    }

    inline std::wstring LocalMachineName()
    {
        wchar_t name[MAX_COMPUTERNAME_LENGTH + 1]{};
        DWORD size = static_cast<DWORD>(std::size(name));
        return GetComputerNameW(name, &size) ? std::wstring{ name, size } : std::wstring{ L"Local" };
    }

    inline void Append(std::wstring& value, std::wstring_view detail)
    {
        if (!detail.empty() && detail != value) { if (!value.empty()) { value += L" · "; }value += detail; }
    }

    inline std::wstring Prefix(std::wstring_view commandline, std::wstring_view source,
                               std::wstring_view profileName, std::wstring_view generatedName,
                               std::wstring_view knownIdentity, std::wstring_view shellLabel,
                               std::wstring_view localMachine)
    {
        if (!knownIdentity.empty())
        {
            std::wstring value{ knownIdentity };
            Append(value, !profileName.empty() && profileName != generatedName ? profileName : shellLabel);
            return value;
        }
        const auto kind = Classify(commandline);
        std::wstring value;
        if (source == L"Windows.Terminal.SSH" || kind == LaunchKind::Ssh) { value = L"SSH profile"; }
        else if (source == L"Windows.Terminal.Wsl" || kind == LaunchKind::Wsl) { value = L"WSL profile"; }
        else if (kind == LaunchKind::LocalShell && (source.empty() || source == L"Windows.Terminal.PowershellCore")) { value = localMachine.empty() ? L"Local" : localMachine; }
        else { value = L"Session"; }
        Append(value, profileName);
        return value;
    }

    inline std::wstring Format(std::wstring_view prefix, std::wstring_view initialTitle,
                               std::wstring_view applicationTitle, bool enabled, bool explicitTitle)
    {
        if (!enabled || explicitTitle || prefix.empty()) { return std::wstring{ applicationTitle }; }
        std::wstring value{ prefix };
        if (applicationTitle != initialTitle) { Append(value, applicationTitle); }
        return value;
    }
}
