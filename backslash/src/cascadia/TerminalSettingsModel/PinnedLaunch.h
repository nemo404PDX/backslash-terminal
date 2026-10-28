// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "PinnedDestinations.h"
#include "SshCommandline.h"
#include "../inc/ConnectionIdentityTitle.h"

namespace Microsoft::Terminal::Backslash::Pins
{
    struct Launch
    {
        std::wstring commandline;
        std::wstring directory;
    };
    inline std::wstring EncodedLocation(const std::string& path)
    {
        std::wstring literal;
        const auto widePath = winrt::to_hstring(path);
        for (auto c : std::wstring_view{ widePath }) { literal += c; if (c == L'\'') { literal += c; } }
        const auto code = L"try { Set-Location -LiteralPath '" + literal + L"' -ErrorAction Stop } catch { [Console]::Error.WriteLine($_.Exception.Message); exit 1 }";
        static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        const auto bytes = reinterpret_cast<const unsigned char*>(code.data());
        const auto size = code.size() * sizeof(wchar_t);
        static_assert(sizeof(wchar_t) == 2);
        std::wstring encoded;
        for (size_t i = 0; i < size; i += 3)
        {
            const uint32_t word = (static_cast<uint32_t>(bytes[i]) << 16) | (i + 1 < size ? bytes[i + 1] << 8 : 0) | (i + 2 < size ? bytes[i + 2] : 0);
            encoded += alphabet[(word >> 18) & 63]; encoded += alphabet[(word >> 12) & 63];
            encoded += i + 1 < size ? alphabet[(word >> 6) & 63] : '=';
            encoded += i + 2 < size ? alphabet[word & 63] : '=';
        }
        return L"pwsh -NoLogo -NoExit -EncodedCommand " + encoded;
    }

    inline Launch PrepareLaunch(const Destination& candidate, std::wstring_view currentCommandline, int32_t sshShell = -1, std::wstring_view sshBase = {})
    {
        const auto pin = Validate(candidate);
        namespace S = winrt::Microsoft::Terminal::Settings::Model::implementation::SshCommandline;
        if (pin.kind == Kind::Local)
        {
            if (::Microsoft::Terminal::Backslash::ConnectionIdentityTitle::Classify(currentCommandline) != ::Microsoft::Terminal::Backslash::ConnectionIdentityTitle::LaunchKind::LocalShell) { throw std::invalid_argument("This local launcher cannot safely reopen a fixed directory."); }
            return { {}, std::wstring{ winrt::to_hstring(pin.path) } };
        }
        const auto args = S::Argv(currentCommandline);
        if (pin.kind == Kind::Wsl)
        {
            std::wstring system; THROW_IF_FAILED(wil::GetSystemDirectoryW<std::wstring>(system));
            if (args.size() != 3 || !til::equals_insensitive_ascii(args[0], system + L"\\wsl.exe") || args[1] != L"-d" || args[2] != std::wstring_view{ winrt::to_hstring(pin.host) }) { throw std::invalid_argument("This WSL launcher cannot safely reopen the pinned distribution/path."); }
            return { {}, std::wstring{ winrt::to_hstring(pin.path) } }; // Existing ConPTY WSL --cd handling.
        }
        std::wstring base{ sshBase };
        if (base.empty() && args.size() == 6) { base = S::Quote(args[0]) + L" " + S::Quote(args[4]); }
        std::wstring exe, host, ignored;
        if (sshShell != 3 || !S::Base(base, exe, host) || host != std::wstring_view{ winrt::to_hstring(pin.host) } || S::Recognize(base, currentCommandline, ignored) != 3)
        {
            throw std::invalid_argument("Fixed SSH paths require a supported SSH profile with PowerShell 7 selected. Advanced commands are preserved.");
        }
        const auto command = S::Build(base, 4, EncodedLocation(pin.path));
        if (!command || command->size() > 32760) { throw std::invalid_argument("The pinned SSH launch command is too long."); }
        return { *command, {} }; // Never use a remote directory as the local ssh.exe cwd.
    }
}
