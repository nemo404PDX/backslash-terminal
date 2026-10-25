// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "ConnectionIdentityTitle.h"

namespace Microsoft::Terminal::Backslash::PowerShellDirectoryIntegration
{
    // Installed only in a newly launched, plain interactive PowerShell 7. The
    // original prompt runs before PSConsoleHostReadLine; it is never replaced.
    // A legacy OSC report is an observation from output, not host authentication.
    inline constexpr std::wstring_view Bootstrap = LR"PS(& {
if ($ExecutionContext.SessionState.LanguageMode -ne 'FullLanguage' -or $PSVersionTable.PSVersion.Major -lt 7) { return }
if (Test-Path variable:global:__BackslashCwdV1) { return }
$invoke = $ExecutionContext.SessionState.InvokeCommand
if ($null -eq $invoke.PSObject.Properties['LocationChangedAction']) { return }
$reader = Get-Command PSConsoleHostReadLine -CommandType Function -ErrorAction Ignore
if ($null -eq $reader) { return }
$state = @{ Reader = $reader.ScriptBlock; Prior = $invoke.LocationChangedAction; Ready = $false; Last = $null }
$emit = {
    param($location)
    try {
        $path = ''
        if ($null -ne $location -and $location.Provider.Name -eq 'FileSystem') { $path = $location.ProviderPath }
        if ($path.Length -gt 32760 -or $path -match '[\x00-\x1f\x7f-\x9f]') { $path = '' }
        if ($path -ceq $state.Last) { return }
        [Console]::Write(([string][char]27) + ']9;9;' + $path + [char]7)
        $state.Last = $path
    } catch { }
}.GetNewClosure()
$invoke.LocationChangedAction = {
    param($sender, $eventArgs)
    try { if ($null -ne $state.Prior) { $state.Prior.Invoke($sender, $eventArgs) } }
    finally { if ($state.Ready) { & $emit $eventArgs.NewPath } else { & $emit $null } }
}.GetNewClosure()
$readLine = {
    $state.Ready = $true
    & $emit $ExecutionContext.SessionState.Path.CurrentLocation
    try { $state.Reader.Invoke() }
    finally { $state.Ready = $false; & $emit $null }
}.GetNewClosure()
Set-Item function:global:PSConsoleHostReadLine -Value $readLine
$global:__BackslashCwdV1 = $state
})PS";

    inline std::wstring Encode(std::wstring_view script)
    {
        static_assert(sizeof(wchar_t) == 2);
        constexpr std::wstring_view alphabet = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::wstring encoded;
        unsigned int bits = 0;
        int count = 0;
        for (const auto ch : script)
        {
            for (const auto byte : { static_cast<unsigned int>(ch & 0xff), static_cast<unsigned int>((ch >> 8) & 0xff) })
            {
                bits = (bits << 8) | byte;
                count += 8;
                while (count >= 6) { count -= 6; encoded.push_back(alphabet[(bits >> count) & 63]); }
            }
        }
        if (count) { encoded.push_back(alphabet[(bits << (6 - count)) & 63]); }
        while (encoded.size() % 4) { encoded.push_back(L'='); }
        return encoded;
    }

    inline std::wstring Prepare(std::wstring_view logicalCommandline)
    {
        namespace C = ConnectionIdentityTitle;
        if (C::Classify(logicalCommandline) != C::LaunchKind::LocalShell) { return std::wstring{ logicalCommandline }; }
        int count = 0;
        const std::wstring copy{ logicalCommandline };
        const wil::unique_hlocal_ptr<wchar_t*[]> args{ CommandLineToArgvW(copy.c_str(), &count) };
        std::wstring exe{ args[0] };
        if (const auto pos = exe.find_last_of(L"\\/"); pos != exe.npos) { exe.erase(0, pos + 1); }
        std::transform(exe.begin(), exe.end(), exe.begin(), [](wchar_t ch) { return static_cast<wchar_t>(towlower(ch)); });
        if (exe != L"pwsh" && exe != L"pwsh.exe") { return copy; }
        bool noExit = false;
        for (int i = 1; i < count; ++i)
        {
            std::wstring flag{ args[i] };
            std::transform(flag.begin(), flag.end(), flag.begin(), [](wchar_t ch) { return static_cast<wchar_t>(towlower(ch)); });
            if (flag == L"-noninteractive") { return copy; }
            noExit = noExit || flag == L"-noexit";
        }
        // Keep argv spelling/quoting intact. Base64 is a single Windows argv
        // atom with no percent expansion, remote-shell quoting or wt escaping.
        static const auto encoded = Encode(Bootstrap);
        auto physical = copy + (noExit ? L"" : L" -NoExit") + L" -EncodedCommand " + encoded;
        return physical.size() <= 32766 ? physical : copy;
    }
}
