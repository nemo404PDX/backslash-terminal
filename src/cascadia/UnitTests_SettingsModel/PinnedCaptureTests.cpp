// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "pch.h"
#include "../TerminalSettingsModel/PinnedCapture.h"
using namespace WEX::TestExecution;
namespace P = Microsoft::Terminal::Backslash::Pins;
namespace SettingsModelUnitTests
{
    class PinnedCaptureTests
    {
        TEST_CLASS(PinnedCaptureTests);
        TEST_METHOD(MissingReportRequiresConfirmation);
        TEST_METHOD(ReportedPathWinsWithoutExpansion);
        TEST_METHOD(UnresolvedReportBecomesEditableSuggestion);
        TEST_METHOD(HomeAndAppDataAreOrdinaryValidFolders);
        TEST_METHOD(ConfirmedLocalTokensAreSavedAsLiterals);
        TEST_METHOD(RemoteNeverUsesLocalSeedOrExpansion);
        TEST_METHOD(CancellationDoesNotCaptureStartupSeed);
        TEST_METHOD(InvalidInputUsesClearContext);
        static std::string expand(const std::string& text)
        {
            if (text == "%USERPROFILE%") { return "C:\\Users\\nemo"; }
            if (text == "%USERPROFILE%\\AppData") { return "C:\\Users\\nemo\\AppData"; }
            if (text == "%APPDATA%") { return "C:\\Users\\nemo\\AppData\\Roaming"; }
            return text;
        }
        static P::Destination pin(const P::CapturedDirectory& folder)
        {
            return P::Capture({ "{11111111-1111-4111-8111-111111111111}", "{22222222-2222-4222-8222-222222222222}", "", folder.kind, "PowerShell", "fixture-machine", folder.path }, folder.origin);
        }
    };
    void PinnedCaptureTests::MissingReportRequiresConfirmation()
    {
        const auto request = P::RequestDirectory(P::Kind::Local, "", "%USERPROFILE%", "C:\\fallback", expand);
        VERIFY_IS_TRUE(request.needsConfirmation); VERIFY_IS_FALSE(request.invalidReport); VERIFY_ARE_EQUAL(std::string{ "C:\\Users\\nemo" }, request.path);
        VERIFY_IS_FALSE(P::FinishDirectory(request, {}, expand).has_value());
        VERIFY_THROWS(P::CurrentDirectory(request), std::exception);
    }
    void PinnedCaptureTests::ReportedPathWinsWithoutExpansion()
    {
        const auto never = [](const std::string&) -> std::string { throw std::runtime_error("A report must not be environment-expanded"); };
        const auto request = P::RequestDirectory(P::Kind::Local, "C:\\Users\\nemo\\AppData", "%PROFILE%", "%USERPROFILE%", never);
        VERIFY_IS_FALSE(request.needsConfirmation); const auto chosen = P::FinishDirectory(request, std::string{ "D:\\unrelated" }, never);
        VERIFY_IS_TRUE(chosen->origin == P::DirectoryOrigin::Reported); VERIFY_ARE_EQUAL(std::string{ "C:\\Users\\nemo\\AppData" }, chosen->path);
    }
    void PinnedCaptureTests::UnresolvedReportBecomesEditableSuggestion()
    {
        const auto request = P::RequestDirectory(P::Kind::Local, "%USERPROFILE%", "%PROFILE%", "%USERPROFILE%", expand);
        VERIFY_IS_TRUE(request.needsConfirmation); VERIFY_IS_TRUE(request.invalidReport); VERIFY_ARE_EQUAL(std::string{ "C:\\Users\\nemo" }, request.path);
        VERIFY_IS_FALSE(P::FinishDirectory(request, {}, expand).has_value());
        VERIFY_THROWS(P::CurrentDirectory(request), std::exception);
        const auto chosen = P::FinishDirectory(request, std::string{ "C:\\Users\\nemo\\AppData" }, expand);
        VERIFY_IS_TRUE(chosen->origin == P::DirectoryOrigin::Confirmed); VERIFY_ARE_EQUAL(std::string{ "C:\\Users\\nemo\\AppData" }, pin(*chosen).path);
    }
    void PinnedCaptureTests::HomeAndAppDataAreOrdinaryValidFolders()
    {
        for (const auto* folder : { "C:\\Users\\nemo", "C:\\Users\\nemo\\AppData", "C:\\Users\\nemo\\AppData\\Roaming", "C:\\Users\\A User\\AppData" })
        {
            VERIFY_IS_TRUE(P::WindowsPath(folder)); const auto chosen = P::ConfirmDirectory(P::Kind::Local, folder, expand);
            VERIFY_ARE_EQUAL(std::string{ folder }, pin(chosen).path);
        }
    }
    void PinnedCaptureTests::ConfirmedLocalTokensAreSavedAsLiterals()
    {
        for (const auto* input : { "%USERPROFILE%", "%USERPROFILE%\\AppData", "%APPDATA%" })
        {
            int expansions = 0; const auto chosen = P::ConfirmDirectory(P::Kind::Local, input, [&](const auto& text) { ++expansions; return expand(text); });
            P::Catalog catalog; catalog.Add(pin(chosen)); const auto original = chosen.path;
            VERIFY_ARE_EQUAL(size_t{ 1 }, P::Catalog::Read(catalog.Write()).pins.size()); VERIFY_ARE_EQUAL(original, P::Catalog::Read(catalog.Write()).pins[0].path); VERIFY_ARE_EQUAL(1, expansions);
            VERIFY_IS_TRUE(original.find('%') == original.npos);
        }
        VERIFY_THROWS(P::ConfirmDirectory(P::Kind::Local, "%PROFILE%", expand), std::exception);
    }
    void PinnedCaptureTests::RemoteNeverUsesLocalSeedOrExpansion()
    {
        const auto never = [](const std::string&) -> std::string { throw std::runtime_error("Local expansion must not run for a remote pin"); };
        for (const auto kind : { P::Kind::Wsl, P::Kind::SshWindows, P::Kind::SshPosix })
        {
            const auto request = P::RequestDirectory(kind, "", "C:\\Users\\nemo", "%USERPROFILE%", never);
            VERIFY_IS_TRUE(request.needsConfirmation); VERIFY_IS_TRUE(request.path.empty());
            const auto chosen = P::FinishDirectory(request, std::string{ "/home/fixture" }, never);
            VERIFY_IS_TRUE(chosen->kind == (kind == P::Kind::Wsl ? kind : P::Kind::SshPosix));
            VERIFY_THROWS(P::ConfirmDirectory(kind, "%USERPROFILE%", never), std::exception);
        }
        const auto windows = P::ConfirmDirectory(P::Kind::SshPosix, "C:\\Users\\remote", never); VERIFY_IS_TRUE(windows.kind == P::Kind::SshWindows);
    }
    void PinnedCaptureTests::CancellationDoesNotCaptureStartupSeed()
    {
        const auto request = P::RequestDirectory(P::Kind::Local, "", "C:\\startup", "C:\\home", expand);
        P::Catalog catalog; const auto before = catalog.Write(); const auto result = P::FinishDirectory(request, std::nullopt, expand);
        VERIFY_IS_FALSE(result.has_value()); VERIFY_ARE_EQUAL(before, catalog.Write());
        auto candidate = pin(P::ConfirmDirectory(P::Kind::Local, "C:\\startup", expand));
        VERIFY_THROWS(P::Capture(candidate, P::DirectoryOrigin::ProfileSeed), std::exception);
        VERIFY_THROWS(P::Capture(candidate, P::DirectoryOrigin::Unknown), std::exception);
    }
    void PinnedCaptureTests::InvalidInputUsesClearContext()
    {
        for (const auto* text : { "", "AppData", "C:\\bad\npath", "C:\\bad|path", "%PROFILE%" }) { VERIFY_THROWS(P::ConfirmDirectory(P::Kind::Local, text, expand), std::exception); }
        const auto trimmed = P::ConfirmDirectory(P::Kind::Local, "  C:\\Users\\nemo\\AppData  ", expand); VERIFY_ARE_EQUAL(std::string{ "C:\\Users\\nemo\\AppData" }, trimmed.path);
        auto invalid = P::RequestDirectory(P::Kind::Local, "relative-report", "C:\\startup", "C:\\home", expand); VERIFY_IS_TRUE(invalid.invalidReport); VERIFY_IS_TRUE(invalid.needsConfirmation);
    }
}
