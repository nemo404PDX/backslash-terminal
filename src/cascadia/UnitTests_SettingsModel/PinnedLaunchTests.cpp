// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "pch.h"
#include "../TerminalSettingsModel/PinnedLaunch.h"
#include "../TerminalSettingsModel/GroupSavePolicy.h"
using namespace WEX::TestExecution;
using namespace winrt::Microsoft::Terminal::Settings::Model;
namespace P = Microsoft::Terminal::Backslash::Pins;
namespace S = winrt::Microsoft::Terminal::Settings::Model::implementation::SshCommandline;
namespace SettingsModelUnitTests
{
    class PinnedLaunchTests
    {
        TEST_CLASS(PinnedLaunchTests);
        TEST_METHOD(LocalAndUnsupportedLaunchers);
        TEST_METHOD(WslUsesExactSystemLauncherAndReportedPath);
        TEST_METHOD(SshEncodedPathDoesNotUseLocalCwdOrEditedName);
        TEST_METHOD(EncodedPowerShellLiteralRoundtrip);
        TEST_METHOD(ReferencesRoundtripWithoutAffectingOrdinaryActions);
        TEST_METHOD(DormantPinsExcludedFromGroups);
        static P::Destination fixture(P::Kind kind)
        {
            P::Destination pin{ "{11111111-1111-4111-8111-111111111111}", "{22222222-2222-4222-8222-222222222222}", "", kind, "Edited display name", "fixture-machine", "C:\\fixture folder" };
            if (kind == P::Kind::Wsl) { pin.source = "Windows.Terminal.Wsl"; pin.host = "FixtureDistro"; pin.path = "/home/fixture/a b;$()'"; }
            if (kind == P::Kind::SshWindows || kind == P::Kind::SshPosix) { pin.source = "Windows.Terminal.SSH"; pin.host = "fixture-alias"; pin.path = kind == P::Kind::SshWindows ? "D:\\fixture folder\\quote'" : "/srv/fixture/a b;$()'"; }
            return pin;
        }
    };
    void PinnedLaunchTests::LocalAndUnsupportedLaunchers()
    {
        const auto pin = fixture(P::Kind::Local);
        const auto launch = P::PrepareLaunch(pin, L"pwsh.exe -NoLogo");
        VERIFY_IS_TRUE(launch.commandline.empty()); VERIFY_IS_TRUE(launch.directory == L"C:\\fixture folder");
        for (auto command : { L"pwsh.exe -Command Invoke-Remote", L"pwsh.exe -HostName fixture", L"cmd.exe /c echo hi", L"wrapper.exe", L"wsl.exe -d Other" }) { VERIFY_THROWS(P::PrepareLaunch(pin, command), std::exception); }
    }
    void PinnedLaunchTests::WslUsesExactSystemLauncherAndReportedPath()
    {
        const auto pin = fixture(P::Kind::Wsl); std::wstring system; THROW_IF_FAILED(wil::GetSystemDirectoryW<std::wstring>(system));
        const auto command = S::Quote(system + L"\\wsl.exe") + L" -d FixtureDistro";
        const auto launch = P::PrepareLaunch(pin, command);
        VERIFY_IS_TRUE(launch.directory == std::wstring_view{ winrt::to_hstring(pin.path) }); VERIFY_IS_TRUE(launch.commandline.empty());
        VERIFY_THROWS(P::PrepareLaunch(pin, L"wsl.exe -d FixtureDistro"), std::exception);
        VERIFY_THROWS(P::PrepareLaunch(pin, command + L" -u root"), std::exception);
        auto replacement = pin; replacement.host = "AnotherDistro"; VERIFY_THROWS(P::PrepareLaunch(replacement, command), std::exception);
    }
    void PinnedLaunchTests::SshEncodedPathDoesNotUseLocalCwdOrEditedName()
    {
        const std::wstring base = L"\"C:\\fixture tools\\ssh.exe\" fixture-alias";
        for (auto kind : { P::Kind::SshWindows, P::Kind::SshPosix })
        {
            const auto pin = fixture(kind); const auto command = *S::Build(base, 3); const auto launch = P::PrepareLaunch(pin, command, 3); const auto args = S::Argv(launch.commandline);
            VERIFY_IS_TRUE(launch.directory.empty()); VERIFY_ARE_EQUAL(size_t{ 6 }, args.size()); VERIFY_IS_TRUE(args[4] == L"fixture-alias");
            VERIFY_IS_TRUE(args[5] == P::EncodedLocation(pin.path)); VERIFY_IS_TRUE(launch.commandline.find(L"Edited display name") == launch.commandline.npos);
            for (auto mode : { 0, 1, 2, 4 }) { VERIFY_THROWS(P::PrepareLaunch(pin, *S::Build(base, mode, L"custom-shell"), mode), std::exception); }
            VERIFY_THROWS(P::PrepareLaunch(pin, command + L" extra", 3), std::exception);
        }
    }
    void PinnedLaunchTests::EncodedPowerShellLiteralRoundtrip()
    {
        const auto command = P::EncodedLocation("/srv/a b;'$(touch secret)"); const auto encoded = command.substr(command.find_last_of(L' ') + 1);
        const std::wstring alphabet = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::vector<unsigned char> bytes; uint32_t value = 0; int bits = 0;
        for (const auto ch : encoded) { if (ch == L'=') { break; } const auto index = alphabet.find(ch); VERIFY_IS_TRUE(index != alphabet.npos); value = (value << 6) | static_cast<uint32_t>(index); bits += 6; if (bits >= 8) { bits -= 8; bytes.push_back(static_cast<unsigned char>(value >> bits)); } }
        VERIFY_IS_TRUE(bytes.size() % 2 == 0); std::wstring decoded;
        for (size_t i = 0; i < bytes.size(); i += 2) { decoded += static_cast<wchar_t>(bytes[i] | (bytes[i + 1] << 8)); }
        VERIFY_IS_TRUE(decoded.find(L"-LiteralPath '/srv/a b;''$(touch secret)' -ErrorAction Stop") != decoded.npos);
        VERIFY_IS_TRUE(decoded.ends_with(L"exit 1 }"));
    }
    void PinnedLaunchTests::ReferencesRoundtripWithoutAffectingOrdinaryActions()
    {
        NewTerminalArgs ordinary; const auto json = ActionAndArgs::Serialize(winrt::single_threaded_vector<ActionAndArgs>({ ActionAndArgs{ ShortcutAction::NewTab, NewTabArgs{ ordinary } } }));
        VERIFY_IS_TRUE(std::wstring_view{ json }.find(L"__backslashPin") == std::wstring_view::npos);
        NewTerminalArgs args; args.PinnedId(L"{11111111-1111-4111-8111-111111111111}"); args.PinnedTransfer(L"{22222222-2222-4222-8222-222222222222}"); args.PinnedBackground(true);
        const auto serialized = ActionAndArgs::Serialize(winrt::single_threaded_vector<ActionAndArgs>({ ActionAndArgs{ ShortcutAction::NewTab, NewTabArgs{ args } } }));
        const auto roundtrip = ActionAndArgs::Deserialize(serialized).GetAt(0).Args().as<NewTabArgs>().ContentArgs().as<NewTerminalArgs>();
        VERIFY_IS_TRUE(args.Equals(roundtrip)); const auto copy = args.Copy().as<NewTerminalArgs>(); VERIFY_IS_TRUE(copy.Equals(args));
    }
    void PinnedLaunchTests::DormantPinsExcludedFromGroups()
    {
        namespace G = Microsoft::Terminal::Backslash::GroupSavePolicy;
        const auto selection = G::Select({ { G::PaneKind::DormantPin }, { G::PaneKind::Terminal }, { G::PaneKind::Settings } }, 0);
        VERIFY_ARE_EQUAL(size_t{ 1 }, selection.indices.size()); VERIFY_ARE_EQUAL(uint32_t{ 1 }, selection.indices[0]); VERIFY_ARE_EQUAL(uint32_t{ 0 }, selection.focusedTab); VERIFY_IS_TRUE(selection.skippedSettings);
        VERIFY_THROWS(G::Select({ { G::PaneKind::DormantPin } }, 0), std::exception);
    }
}
