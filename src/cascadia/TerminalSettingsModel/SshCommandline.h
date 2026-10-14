// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

#include <shellapi.h>

namespace winrt::Microsoft::Terminal::Settings::Model::implementation::SshCommandline
{
    inline bool SingleLine(std::wstring_view value)
    {
        return value.find_first_of(std::wstring_view{ L"\0\r\n", 3 }) == value.npos;
    }

    // ConPTY expands %variables% before CreateProcess. Reject % in literal targets/commands.
    inline bool ValidHost(std::wstring_view host)
    {
        return !host.empty() && host.front() != L'-' && SingleLine(host) &&
               host.find_first_of(L" \t%*?!\"\\") == host.npos;
    }

    // Windows argv quoting, independent of the remote login shell and wt's ; syntax.
    inline std::wstring Quote(std::wstring_view value)
    {
        std::wstring result{ L"\"" };
        size_t slashes = 0;
        for (const auto ch : value)
        {
            if (ch == L'\\')
            {
                ++slashes;
                continue;
            }
            result.append(slashes * (ch == L'"' ? 2 : 1), L'\\');
            slashes = 0;
            if (ch == L'"')
            {
                result += L'\\';
            }
            result += ch;
        }
        result.append(slashes * 2, L'\\');
        result += L'"';
        return result;
    }

    inline std::vector<std::wstring> Argv(std::wstring_view command)
    {
        if (!SingleLine(command) || command.empty())
        {
            return {};
        }
        int count = 0;
        wil::unique_hlocal_ptr<wchar_t*> args{ CommandLineToArgvW(std::wstring{ command }.c_str(), &count) };
        std::vector<std::wstring> result;
        if (args)
        {
            for (int i = 0; i < count; ++i)
            {
                result.emplace_back(args.get()[i]);
            }
        }
        return result;
    }

    inline bool Base(std::wstring_view command, std::wstring& exe, std::wstring& host)
    {
        const auto args = Argv(command);
        if (args.size() != 2 || !ValidHost(args[1]) ||
            !til::equals_insensitive_ascii(std::filesystem::path{ args[0] }.filename().wstring(), L"ssh.exe"))
        {
            return false;
        }
        exe = args[0];
        host = args[1];
        return true;
    }

    inline std::optional<std::wstring> Build(std::wstring_view base, int32_t mode, std::wstring_view custom = {})
    {
        std::wstring exe, host;
        if (!Base(base, exe, host) || mode < 0 || mode > 4)
        {
            return std::nullopt;
        }
        if (mode == 0)
        {
            return std::wstring{ base };
        }
        auto result = Quote(exe) + L" -t -o RemoteCommand=none " + Quote(host);
        if (mode > 1)
        {
            const auto remote = mode == 2 ? std::wstring_view{ L"bash -l" } :
                                mode == 3 ? std::wstring_view{ L"pwsh -NoLogo" } : custom;
            if (remote.find_first_not_of(L" \t") == remote.npos || !SingleLine(remote) || remote.find(L'%') != remote.npos)
            {
                return std::nullopt;
            }
            result += L" " + Quote(remote);
        }
        return result;
    }

    inline int32_t Recognize(std::wstring_view base, std::wstring_view command, std::wstring& custom)
    {
        for (int32_t mode = 0; mode < 4; ++mode)
        {
            if (const auto expected = Build(base, mode); expected && *expected == command)
            {
                return mode;
            }
        }
        const auto args = Argv(command);
        if (args.size() == 6)
        {
            if (const auto expected = Build(base, 4, args.back()); expected && *expected == command)
            {
                custom = args.back();
                return 4;
            }
        }
        return -1; // Unfamiliar advanced commands are never rewritten by the selector.
    }
}
