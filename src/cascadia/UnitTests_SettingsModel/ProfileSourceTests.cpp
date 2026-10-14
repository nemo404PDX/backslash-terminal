// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "pch.h"
#include <cmath>
#include "JsonTestClass.h"
#include "../TerminalSettingsModel/CascadiaSettings.h"
#include "../TerminalSettingsModel/ApplicationState.h"
#include "../TerminalSettingsModel/SshHostGenerator.h"
#include "../TerminalSettingsModel/WslDistroGenerator.h"
#include "../TerminalSettingsModel/SshCommandline.h"
#include "../TerminalSettingsModel/DynamicProfileUtils.h"
#include "../inc/RetroBezelGeometry.h"
#include "../inc/AppearancePreviewLayout.h"
#include <limits>
#include "../inc/BezelBadgeMaterial.h"
#include "../inc/TerminalTabPolicy.h"

using namespace winrt::Microsoft::Terminal::Settings::Model;
using namespace WEX::TestExecution;
using namespace WEX::Logging;

namespace SettingsModelUnitTests
{
    class FixtureSshGenerator : public IDynamicProfileGenerator
    {
    public:
        std::wstring exePath{ L"%SystemRoot%\\System32\\OpenSSH\\ssh.exe" };
        std::vector<std::wstring> hosts{ L"sameHost", L"unrelatedHost" };
        std::wstring_view GetNamespace() const noexcept override { return L"Windows.Terminal.SSH"; }
        std::wstring_view GetDisplayName() const noexcept override { return L"Fixture SSH"; }
        std::wstring_view GetIcon() const noexcept override { return {}; }
        void GenerateProfiles(std::vector<winrt::com_ptr<implementation::Profile>>& profiles) const override
        {
            SshHostGenerator::GenerateForHosts(exePath, hosts, profiles);
        }
    };

    class ProfileSourceTests : public JsonTestClass
    {
        TEST_CLASS(ProfileSourceTests);
        TEST_METHOD(BackslashSshCommandConstruction);
        TEST_METHOD(BackslashSshShellRoundtrip);
        TEST_METHOD(BackslashSshRestoreLifecycle);
        TEST_METHOD(BackslashSshRestoreHiddenAndOrphan);
        TEST_METHOD(BackslashSshScanCancelAndFailedSave);
        TEST_METHOD(BackslashModernWslDiscovery);
        TEST_METHOD(BackslashBezelSquareOuterRoundedInner);
        TEST_METHOD(BackslashBezelCenteringModeAndTabs);
        TEST_METHOD(BackslashBezelOpaqueSurroundTransparentScreen);
        TEST_METHOD(BackslashBezelBadgeVerticalChinCenter);
        TEST_METHOD(BackslashBezelBadgeMaterialSwatchesAndDpi);
        TEST_METHOD(BackslashBezelDarkPalettesContrastAndIsolation);
        TEST_METHOD(BackslashBezelRecessCornerRadii);
        TEST_METHOD(BackslashBezelThickerPresetDpiLayout);
        TEST_METHOD(BackslashBezelSubstantialRecessGeometry);
        TEST_METHOD(BackslashBezelAppearancePreviewViewportSizing);
        TEST_METHOD(BackslashBezelAppearancePreviewDefaultsAndOverrides);
        TEST_METHOD(BackslashBezelDirectionalMaterialLighting);
        TEST_METHOD(BackslashSshSavedLaunchArgv);

        static constexpr std::string_view localJSON = R"({"profiles":{"list":[{"name":"local","guid":"{6239a42c-1de4-49a3-80bd-e8fdd045185c}","commandline":"cmd.exe"}]}})";

        static winrt::com_ptr<implementation::ApplicationState> tempState()
        {
            GUID testGuid;
            THROW_IF_FAILED(CoCreateGuid(&testGuid));
            const auto directory = std::filesystem::temp_directory_path() / (L"Backslash-profile-tests-" + std::wstring{ winrt::to_hstring(testGuid) });
            std::filesystem::create_directory(directory);
            Log::Comment(directory.c_str());
            return winrt::make_self<implementation::ApplicationState>(directory);
        }

        static winrt::com_ptr<implementation::CascadiaSettings> load(std::string_view json, const FixtureSshGenerator& generator, implementation::ApplicationState& state)
        {
            implementation::SettingsLoader loader{ json, "{}" };
            loader.inboxSettings.profiles = loader.GenerateProfilesFor(generator);
            loader.MergeInboxIntoUserSettings();
            loader.FinalizeLayering();
            loader.DisableDeletedProfiles(state);
            loader.FixupUserSettings();
            return winrt::make_self<implementation::CascadiaSettings>(std::move(loader));
        }

        static winrt::guid hostGuid(std::wstring_view host)
        {
            return CreateDynamicProfile(std::wstring{ L"SSH - " } + std::wstring{ host })->Guid();
        }

        static void remove(const winrt::com_ptr<implementation::CascadiaSettings>& settings, winrt::guid guid)
        {
            uint32_t index;
            const auto profile = settings->FindProfile(guid);
            if (settings->AllProfiles().IndexOf(profile, index))
            {
                settings->AllProfiles().RemoveAt(index);
            }
        }

        static size_t count(const winrt::com_ptr<implementation::CascadiaSettings>& settings, winrt::guid guid)
        {
            size_t result = 0;
            for (const auto& p : settings->AllProfiles())
            {
                result += p.Guid() == guid;
            }
            return result;
        }
    };

    void ProfileSourceTests::BackslashSshCommandConstruction()
    {
        using namespace implementation::SshCommandline;
        const auto base = L"\"C:\\Program Files\\OpenSSH\\ssh.exe\" original-alias";
        VERIFY_ARE_EQUAL(std::wstring{ base }, *Build(base, 0));
        VERIFY_ARE_EQUAL(std::wstring{ L"\"C:\\Program Files\\OpenSSH\\ssh.exe\" -t -o RemoteCommand=none \"original-alias\"" }, *Build(base, 1));
        const auto args = Argv(*Build(base, 3));
        VERIFY_ARE_EQUAL(size_t{ 6 }, args.size());
        VERIFY_ARE_EQUAL(std::wstring{ L"pwsh -NoLogo" }, args[5]);
        VERIFY_ARE_EQUAL(std::wstring{ L"original-alias" }, args[4]);
        const auto custom = LR"(pwsh -NoLogo -Command "Write-Output 'a;b'"; echo C:\tail\)";
        const auto command = Build(base, 4, custom);
        VERIFY_IS_TRUE(command.has_value());
        VERIFY_ARE_EQUAL(std::wstring{ custom }, Argv(*command).back());
        std::wstring recovered;
        VERIFY_ARE_EQUAL(4, Recognize(base, *command, recovered));
        VERIFY_ARE_EQUAL(std::wstring{ custom }, recovered);
        for (auto bad : { std::wstring{ L"" }, std::wstring{ L"echo %USERPROFILE%" }, std::wstring{ L"echo\nnext" }, std::wstring{ L"echo\rnext" }, std::wstring{ L"a\0b", 3 } })
        {
            VERIFY_IS_FALSE(Build(base, 4, bad).has_value());
        }
        VERIFY_IS_FALSE(Build(base, 4, L" \t").has_value());
        VERIFY_IS_FALSE(Build(L"ssh.exe -host", 2).has_value());
        VERIFY_IS_FALSE(Build(L"ssh.exe %HOST%", 2).has_value());
        VERIFY_IS_FALSE(Build(L"ssh.exe wildcard*", 2).has_value());
        VERIFY_IS_FALSE(Build(L"ssh.exe host\n", 2).has_value());
        const auto expanded = wil::ExpandEnvironmentStringsW<std::wstring>(Build(L"\"%SystemRoot%\\System32\\OpenSSH\\ssh.exe\" alias", 4, custom)->c_str());
        VERIFY_ARE_EQUAL(std::wstring{ custom }, Argv(expanded).back());
    }

    void ProfileSourceTests::BackslashSshShellRoundtrip()
    {
        auto state = tempState();
        FixtureSshGenerator generator;
        auto settings = load(localJSON, generator, *state);
        const auto guid = hostGuid(L"sameHost");
        auto profile = settings->FindProfile(guid);
        profile.Name(L"renamed display / 192.0.2.1");
        VERIFY_IS_TRUE(profile.HasSshShellSelector());
        VERIFY_ARE_EQUAL(0, profile.SshShell());
        for (int32_t mode = 0; mode <= 4; ++mode)
        {
            VERIFY_IS_TRUE(profile.SetSshShell(mode, L"exec zsh -l; echo 'quoted text'"));
            VERIFY_ARE_EQUAL(mode, profile.SshShell());
            auto reloaded = load(toString(settings->ToJson()), generator, *state);
            const auto saved = reloaded->FindProfile(guid);
            VERIFY_ARE_EQUAL(profile.Commandline(), saved.Commandline());
            VERIFY_ARE_EQUAL(mode, saved.SshShell());
            VERIFY_ARE_EQUAL(guid, saved.Guid());
            VERIFY_ARE_EQUAL(winrt::hstring{ L"Windows.Terminal.SSH" }, saved.Source());
            if (mode != 0)
            {
                VERIFY_ARE_EQUAL(std::wstring{ L"sameHost" }, implementation::SshCommandline::Argv(saved.Commandline())[4]);
            }
        }
        profile.Commandline(L"ssh.exe -J jumpHost sameHost advanced -option");
        const auto advanced = profile.Commandline();
        VERIFY_ARE_EQUAL(-1, profile.SshShell());
        VERIFY_IS_FALSE(profile.SetSshShell(2, L""));
        VERIFY_ARE_EQUAL(advanced, profile.Commandline());
        profile.ClearCommandline();
        VERIFY_ARE_EQUAL(0, profile.SshShell());
        VERIFY_IS_TRUE(profile.SetSshShell(3, L""));
        const auto before = profile.Commandline();
        VERIFY_IS_FALSE(profile.SetSshShell(4, L"echo %PATH%"));
        VERIFY_ARE_EQUAL(before, profile.Commandline());
    }

    void ProfileSourceTests::BackslashSshRestoreLifecycle()
    {
        auto state = tempState();
        FixtureSshGenerator generator;
        const auto guid = hostGuid(L"sameHost");
        const auto other = hostGuid(L"unrelatedHost");
        auto original = load(localJSON, generator, *state);
        original->FindProfile(guid).HistorySize(123);
        auto saved = toString(original->ToJson());
        // Remove Host, delete the retained orphan, save, then reintroduce identical Host.
        generator.hosts = { L"unrelatedHost" };
        auto missing = load(saved, generator, *state);
        VERIFY_IS_TRUE(missing->FindProfile(guid).Orphaned());
        remove(missing, guid);
        remove(missing, other); // A still-present, intentionally deleted unrelated host.
        saved = toString(missing->ToJson());
        generator.hosts = { L"sameHost", L"unrelatedHost", L"sameHost" };
        auto deleted = load(saved, generator, *state);
        VERIFY_IS_TRUE(deleted->FindProfile(guid).Deleted());
        VERIFY_IS_TRUE(deleted->FindProfile(other).Deleted());
        auto clone = deleted->Copy();
        const auto edit = winrt::get_self<implementation::CascadiaSettings>(clone);
        const auto seen = state->GeneratedProfiles();
        edit->ScanSshProfiles(generator);
        VERIFY_IS_TRUE(clone.FindProfile(guid).Deleted()); // Scan is read-only.
        const auto restored = clone.RestoreSshProfile(guid);
        VERIFY_IS_NOT_NULL(restored);
        VERIFY_IS_FALSE(restored.Deleted());
        VERIFY_IS_FALSE(restored.Hidden());
        VERIFY_IS_FALSE(restored.Orphaned());
        VERIFY_ARE_EQUAL(9001, restored.HistorySize()); // Defaults, not deleted customization.
        VERIFY_IS_TRUE(restored.HasSshShellSelector());
        VERIFY_IS_TRUE(clone.FindProfile(other).Deleted());
        VERIFY_IS_TRUE(state->GeneratedProfiles() == seen);
        VERIFY_IS_TRUE(deleted->FindProfile(guid).Deleted()); // Source untouched until Save.
        clone.RestoreSshProfile(guid);
        size_t matching = 0;
        for (const auto& p : clone.AllProfiles()) { matching += p.Guid() == guid; }
        VERIFY_ARE_EQUAL(size_t{ 1 }, matching);
        size_t activeMatching = 0;
        for (const auto& p : clone.ActiveProfiles()) { activeMatching += p.Guid() == guid; }
        VERIFY_ARE_EQUAL(size_t{ 1 }, activeMatching);
        size_t generatedParents = 0;
        for (const auto& p : winrt::get_self<implementation::Profile>(restored)->Parents())
        {
            generatedParents += p->Source() == L"Windows.Terminal.SSH" && p->Guid() == guid && p->Origin() == OriginTag::Generated;
        }
        VERIFY_ARE_EQUAL(size_t{ 1 }, generatedParents);
        saved = toString(edit->ToJson());
        auto afterSave = load(saved, generator, *state);
        VERIFY_IS_FALSE(afterSave->FindProfile(guid).Deleted());
        VERIFY_IS_TRUE(afterSave->FindProfile(other).Deleted());
        VERIFY_ARE_EQUAL(size_t{ 1 }, count(afterSave, guid));
        // Second deletion after restoration, with Host still present, stays suppressed.
        remove(afterSave, guid);
        auto secondDelete = load(toString(afterSave->ToJson()), generator, *state);
        VERIFY_IS_TRUE(secondDelete->FindProfile(guid).Deleted());
        secondDelete->ScanSshProfiles(generator);
        VERIFY_IS_NOT_NULL(secondDelete->RestoreSshProfile(guid));
        VERIFY_IS_TRUE(secondDelete->FindProfile(other).Deleted());
        VERIFY_ARE_EQUAL(size_t{ 1 }, count(secondDelete, guid));
    }

    void ProfileSourceTests::BackslashSshRestoreHiddenAndOrphan()
    {
        auto state = tempState();
        FixtureSshGenerator generator;
        const auto guid = hostGuid(L"sameHost");
        auto settings = load(localJSON, generator, *state);
        settings->FindProfile(guid).Hidden(true);
        settings->FindProfile(guid).HistorySize(321);
        settings->FindProfile(guid).Name(L"custom name");
        const auto saved = toString(settings->ToJson());
        generator.hosts = {};
        auto orphan = load(saved, generator, *state);
        VERIFY_IS_TRUE(orphan->FindProfile(guid).Orphaned());
        generator.hosts = { L"sameHost" };
        orphan->ScanSshProfiles(generator);
        const auto restored = orphan->RestoreSshProfile(guid);
        VERIFY_IS_FALSE(restored.Orphaned());
        VERIFY_IS_FALSE(restored.Hidden());
        VERIFY_ARE_EQUAL(321, restored.HistorySize());
        VERIFY_ARE_EQUAL(winrt::hstring{ L"custom name" }, restored.Name());
        VERIFY_IS_TRUE(restored.SetSshShell(2, L""));
        VERIFY_ARE_EQUAL(size_t{ 1 }, count(orphan, guid));
    }

    void ProfileSourceTests::BackslashSshScanCancelAndFailedSave()
    {
        auto state = tempState();
        FixtureSshGenerator generator;
        const auto guid = hostGuid(L"sameHost");
        auto source = load(localJSON, generator, *state);
        remove(source, guid);
        const auto diskBefore = toString(source->ToJson());
        source = load(diskBefore, generator, *state);
        const auto sourceBefore = toString(source->ToJson());
        auto clone = source->Copy();
        const auto edit = winrt::get_self<implementation::CascadiaSettings>(clone);
        clone.FindProfile(hostGuid(L"unrelatedHost")).Name(L"unsaved edit");
        const auto stateBefore = state->ToJson(implementation::FileSource::Shared);
        generator.hosts.push_back(L"freshHost");
        const auto fresh = hostGuid(L"freshHost");
        VERIFY_ARE_EQUAL(uint32_t{ 3 }, edit->ScanSshProfiles(generator).Size());
        VERIFY_IS_NOT_NULL(clone.RestoreSshProfile(fresh));
        VERIFY_IS_FALSE(state->GeneratedProfiles().contains(fresh));
        VERIFY_ARE_EQUAL(winrt::hstring{ L"unsaved edit" }, clone.FindProfile(hostGuid(L"unrelatedHost")).Name());
        clone.RestoreSshProfile(guid);
        // A failure in the same Save writer boundary cannot alter settings source or seen IDs.
        VERIFY_IS_FALSE(edit->WriteSettingsToDisk([](std::string_view) { throw winrt::hresult_error(E_ACCESSDENIED); }, *state));
        VERIFY_ARE_EQUAL(sourceBefore, toString(source->ToJson()));
        VERIFY_IS_TRUE(stateBefore == state->ToJson(implementation::FileSource::Shared));
        VERIFY_IS_TRUE(source->FindProfile(guid).Deleted());
        clone = nullptr; // Cancel discards the editor clone.
        VERIFY_IS_TRUE(source->FindProfile(guid).Deleted());
        auto next = source->Copy();
        const auto nextEdit = winrt::get_self<implementation::CascadiaSettings>(next);
        nextEdit->ScanSshProfiles(generator);
        next.RestoreSshProfile(guid);
        next.RestoreSshProfile(fresh);
        std::string saved;
        VERIFY_IS_TRUE(nextEdit->WriteSettingsToDisk([&](std::string_view value) { saved = value; }, *state));
        VERIFY_IS_TRUE(state->GeneratedProfiles().contains(fresh));
        auto reloaded = load(saved, generator, *state);
        remove(reloaded, fresh);
        const auto freshDeleted = load(toString(reloaded->ToJson()), generator, *state);
        VERIFY_IS_TRUE(freshDeleted->FindProfile(fresh).Deleted());
        VERIFY_IS_FALSE(reloaded->FindProfile(guid).Deleted());
        auto disabled = winrt::make_self<implementation::CascadiaSettings>(R"({"disabledProfileSources":["Windows.Terminal.SSH"],"profiles":[{"name":"local"}]})");
        VERIFY_ARE_EQUAL(uint32_t{ 0 }, disabled->ScanSshProfiles(generator).Size());
        VERIFY_IS_NULL(disabled->RestoreSshProfile(guid));
        generator.hosts = {};
        nextEdit->ScanSshProfiles(generator);
        VERIFY_IS_NULL(next.RestoreSshProfile(guid)); // No stale candidates from previous scan.
    }

    void ProfileSourceTests::BackslashBezelSquareOuterRoundedInner()
    {
        using namespace Microsoft::Terminal::Backslash;
        VERIFY_ARE_EQUAL(0.0f, BezelOuterRadius);
        VERIFY_IS_TRUE(BezelInnerRadius > 0);
        VERIFY_IS_TRUE(BezelOpeningContainsInsetCorner(BezelInnerRadius, BezelInnerClearance));
        for (int size = 0; size < 3; ++size)
        {
            const auto frame = GetBezelInsets(true, size, 0);
            VERIFY_IS_TRUE(frame.left - BezelInnerClearance - 3 > 0);
        }
    }

    void ProfileSourceTests::BackslashBezelCenteringModeAndTabs()
    {
        using namespace Microsoft::Terminal::Backslash;
        auto settings = winrt::make_self<implementation::CascadiaSettings>(localJSON);
        const auto profile = settings->AllProfiles().GetAt(0);
        const auto global = settings->GlobalSettings();
        profile.RetroBezelEnabled(true);
        profile.RetroBezelBadge(RetroBezelBadge::Bottom);
        // Windowed, maximized, fullscreen, and exiting fullscreen. Repeated profile
        // toggles must leave mode and fullscreen-tab preference intact.
        for (const auto mode : { LaunchMode::DefaultMode, LaunchMode::MaximizedMode, LaunchMode::FullscreenMode, LaunchMode::DefaultMode })
        {
            global.LaunchMode(mode);
            const auto fullscreen = mode == LaunchMode::FullscreenMode;
            for (const auto showFullscreenTabs : { false, true })
            {
                global.ShowTabsFullscreen(showFullscreenTabs);
                for (const auto center : { false, true, false, true, false })
                {
                    profile.RetroBezelCenterFullscreen(center);
                    const auto insets = GetBezelInsetsForMode(true, 1, 1, profile.RetroBezelCenterFullscreen(), fullscreen);
                    VERIFY_ARE_EQUAL(center && fullscreen ? 108.0f : 80.0f, insets.top);
                    VERIFY_ARE_EQUAL(108.0f, insets.bottom);
                    VERIFY_ARE_EQUAL(mode, global.LaunchMode());
                    VERIFY_ARE_EQUAL(showFullscreenTabs, global.ShowTabsFullscreen());
                    // The actual production tab policy preserves genuine fullscreen/focus behavior.
                    VERIFY_ARE_EQUAL(!fullscreen || showFullscreenTabs,
                                     Microsoft::Terminal::ShouldShowTerminalTabs(false, fullscreen, global.ShowTabsFullscreen(), true, false, true));
                    VERIFY_IS_FALSE(Microsoft::Terminal::ShouldShowTerminalTabs(true, fullscreen, global.ShowTabsFullscreen(), true, true, true));
                }
            }
        }
        const auto topCentered = GetBezelInsetsForMode(true, 1, 2, true, true);
        VERIFY_ARE_EQUAL(108.0f, topCentered.top);
        VERIFY_ARE_EQUAL(108.0f, topCentered.bottom);
        VERIFY_ARE_EQUAL(160.0f, GetBezelInsetsForMode(true, 1, 0, true, true).Height());
        VERIFY_ARE_EQUAL(0.0f, GetBezelInsetsForMode(false, 1, 1, true, true).Height());
        VERIFY_IS_FALSE(Microsoft::Terminal::ShouldShowTerminalTabs(false, false, false, false, false, false));
        VERIFY_IS_TRUE(Microsoft::Terminal::ShouldShowTerminalTabs(false, false, false, false, true, false));
    }

    void ProfileSourceTests::BackslashBezelOpaqueSurroundTransparentScreen()
    {
        using namespace Microsoft::Terminal::Backslash;
        auto settings = winrt::make_self<implementation::CascadiaSettings>(localJSON);
        const auto profile = settings->AllProfiles().GetAt(0);
        const auto appearance = profile.DefaultAppearance();
        for (const auto opacity : { 0.0f, 0.25f, 0.6f, 1.0f })
        {
            appearance.Opacity(opacity);
            for (const auto acrylic : { false, true })
            {
                appearance.UseAcrylic(acrylic);
                for (int size = 0; size < 3; ++size)
                {
                    for (int badge = 0; badge < 3; ++badge)
                    {
                        for (const auto fullscreen : { false, true })
                        {
                            for (const auto center : { false, true })
                            {
                                const auto insets = GetBezelInsetsForMode(true, size, badge, center, fullscreen);
                                const auto screen = GetBezelScreenBounds(800, 600, insets);
                                // Fixed opaque black backing outside the actual TermControl rectangle.
                                VERIFY_ARE_EQUAL(0xff000000u, BezelSurroundArgb);
                                VERIFY_IS_FALSE(screen.Contains(screen.left - 1, screen.top + 10));
                                VERIFY_IS_FALSE(screen.Contains(screen.left + 10, screen.top - 1));
                                VERIFY_IS_FALSE(screen.Contains(screen.right + 1, screen.top + 10));
                                VERIFY_IS_FALSE(screen.Contains(screen.left + 10, screen.bottom + 1));
                                VERIFY_IS_FALSE(screen.Contains(screen.left - BezelInnerClearance / 2, screen.top - BezelInnerClearance / 2));
                                // All four terminal corners and the center remain holes in the backing.
                                VERIFY_IS_TRUE(screen.Contains(screen.left, screen.top));
                                VERIFY_IS_TRUE(screen.Contains(screen.right - 1, screen.top));
                                VERIFY_IS_TRUE(screen.Contains(screen.left, screen.bottom - 1));
                                VERIFY_IS_TRUE(screen.Contains(screen.right - 1, screen.bottom - 1));
                                VERIFY_IS_TRUE(screen.Contains(400, 300));
                                VERIFY_ARE_EQUAL(opacity, appearance.Opacity());
                                VERIFY_ARE_EQUAL(acrylic, appearance.UseAcrylic());
                            }
                        }
                    }
                }
            }
        }
        const auto disabled = GetBezelScreenBounds(800, 600, GetBezelInsetsForMode(false, 1, 1, true, true));
        VERIFY_IS_TRUE(disabled.Contains(0, 0));
        VERIFY_IS_TRUE(disabled.Contains(799, 599));
    }

    void ProfileSourceTests::BackslashBezelBadgeVerticalChinCenter()
    {
        using namespace Microsoft::Terminal::Backslash;
        for (int size = 0; size < 3; ++size)
        {
            for (int badge = 1; badge <= 2; ++badge)
            {
                for (const auto fullscreen : { false, true })
                {
                    for (const auto center : { false, true })
                    {
                        const auto insets = GetBezelInsetsForMode(true, size, badge, center, fullscreen);
                        const auto side = badge == 2 ? insets.top : insets.bottom;
                        const auto flatStart = BezelOuterRimThickness;
                        const auto flatEnd = side - BezelInnerClearance - BezelRecessThickness;
                        const auto offset = GetBezelBadgeOuterOffset(side);
                        VERIFY_IS_TRUE(offset >= flatStart);
                        VERIFY_IS_TRUE(offset + BezelBadgeHeight <= flatEnd);
                        VERIFY_ARE_EQUAL((flatStart + flatEnd) / 2, offset + BezelBadgeHeight / 2);
                        for (const auto dpiScale : { 1.0f, 1.25f, 1.5f, 2.0f })
                        {
                            VERIFY_ARE_EQUAL(((flatStart + flatEnd) / 2) * dpiScale,
                                             (offset + BezelBadgeHeight / 2) * dpiScale);
                        }
                    }
                }
            }
        }
        // The old fixed 8 DIP position was wrong for both the slim and wide chins.
        VERIFY_ARE_EQUAL(24.0f, GetBezelBadgeOuterOffset(92));
        VERIFY_ARE_EQUAL(32.0f, GetBezelBadgeOuterOffset(108));
        VERIFY_ARE_EQUAL(40.0f, GetBezelBadgeOuterOffset(124));
    }

    void ProfileSourceTests::BackslashSshSavedLaunchArgv()
    {
        wchar_t fakeExe[2048];
        const auto length = GetEnvironmentVariableW(L"BACKSLASH_TEST_SSH_EXE", fakeExe, 2048);
        if (!length || length >= 2048)
        {
            VERIFY_FAIL(L"Build SshCaptureClient.cpp and set BACKSLASH_TEST_SSH_EXE for this fixture.");
            return;
        }
        const auto exe = std::filesystem::path{ fakeExe };
        std::ifstream binary{ exe, std::ios::binary };
        const std::string exeBytes{ std::istreambuf_iterator<char>{ binary }, std::istreambuf_iterator<char>{} };
        if (exeBytes.find("BACKSLASH LOCAL SSH ARGUMENT CAPTURE FIXTURE") == std::string::npos)
        {
            VERIFY_FAIL(L"Refusing to execute anything except the local SSH capture fixture.");
            return;
        }
        auto state = tempState();
        FixtureSshGenerator generator;
        generator.exePath = fakeExe;
        auto settings = load(localJSON, generator, *state);
        const auto guid = hostGuid(L"sameHost");
        for (int32_t mode = 0; mode <= 4; ++mode)
        {
            const auto profile = settings->FindProfile(guid);
            profile.Name(L"display name cannot redirect SSH");
            const auto custom = LR"(pwsh -NoLogo -Command "Write-Output 'a;b'")";
            VERIFY_IS_TRUE(profile.SetSshShell(mode, custom));
            const auto savedPath = exe.parent_path() / (L"ssh-launch-mode-" + std::to_wstring(mode) + L"-settings.json");
            VERIFY_IS_TRUE(settings->WriteSettingsToDisk([&](std::string_view content) {
                std::ofstream saved{ savedPath, std::ios::binary };
                saved.write(content.data(), static_cast<std::streamsize>(content.size()));
                if (!saved.good()) { throw winrt::hresult_error(E_FAIL); }
            }, *state));
            std::ifstream saved{ savedPath, std::ios::binary };
            const std::string contents{ std::istreambuf_iterator<char>{ saved }, std::istreambuf_iterator<char>{} };
            auto reloaded = load(contents, generator, *state);
            const auto restored = reloaded->FindProfile(guid);
            VERIFY_ARE_EQUAL(mode, restored.SshShell());
            VERIFY_ARE_EQUAL(profile.Commandline(), restored.Commandline());
            const auto capturePath = exe.parent_path() / (L"ssh-launch-mode-" + std::to_wstring(mode) + L"-argv.json");
            auto command = wil::ExpandEnvironmentStringsW<std::wstring>(restored.Commandline().c_str());
            std::wstring environment = L"BACKSLASH_TEST_SSH_CAPTURE_PATH=" + capturePath.wstring();
            environment.push_back(L'\0');
            environment.push_back(L'\0');
            STARTUPINFOW startup{ sizeof(startup) };
            PROCESS_INFORMATION process{};
            THROW_IF_WIN32_BOOL_FALSE(CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE,
                                                    CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
                                                    environment.data(), exe.parent_path().c_str(), &startup, &process));
            wil::unique_handle processHandle{ process.hProcess };
            wil::unique_handle threadHandle{ process.hThread };
            VERIFY_ARE_EQUAL(DWORD{ WAIT_OBJECT_0 }, WaitForSingleObject(processHandle.get(), 5000));
            DWORD exitCode;
            THROW_IF_WIN32_BOOL_FALSE(GetExitCodeProcess(processHandle.get(), &exitCode));
            VERIFY_ARE_EQUAL(DWORD{ 0 }, exitCode);
            std::ifstream captured{ capturePath, std::ios::binary };
            const std::string argvText{ std::istreambuf_iterator<char>{ captured }, std::istreambuf_iterator<char>{} };
            const auto argv = VerifyParseSucceeded(argvText);
            VERIFY_ARE_EQUAL(mode == 0 ? Json::ArrayIndex{ 2 } : mode == 1 ? Json::ArrayIndex{ 5 } : Json::ArrayIndex{ 6 }, argv.size());
            VERIFY_ARE_EQUAL(std::string{ "sameHost" }, argv[mode == 0 ? 1 : 4].asString());
            if (mode > 0)
            {
                VERIFY_ARE_EQUAL(std::string{ "-t" }, argv[1].asString());
                VERIFY_ARE_EQUAL(std::string{ "-o" }, argv[2].asString());
                VERIFY_ARE_EQUAL(std::string{ "RemoteCommand=none" }, argv[3].asString());
            }
            if (mode > 1)
            {
                const auto remote = mode == 2 ? L"bash -l" : mode == 3 ? L"pwsh -NoLogo" : custom;
                VERIFY_ARE_EQUAL(til::u16u8(remote), argv[5].asString());
            }
        }
    }

    void ProfileSourceTests::BackslashBezelBadgeMaterialSwatchesAndDpi()
    {
        using namespace Microsoft::Terminal::Backslash;
        auto brightness = [](std::uint32_t color) { return (color & 255u) + ((color >> 8) & 255u) + ((color >> 16) & 255u); };
        std::uint32_t previousFace = 0;
        for (int color = 0; color < 4; ++color)
        {
            const auto casing = BezelPaletteArgb(color);
            const auto plate = GetBezelBadgeMaterial(color);
            VERIFY_ARE_EQUAL(255u, plate.face >> 24);
            VERIFY_ARE_EQUAL(255u, plate.seam >> 24);
            VERIFY_ARE_EQUAL(255u, plate.lowerSeam >> 24);
            VERIFY_ARE_EQUAL(255u, plate.lettering >> 24);
            VERIFY_IS_TRUE(brightness(plate.face) < brightness(casing));
            VERIFY_IS_TRUE(brightness(plate.face) < brightness(BlendBezelRgb(casing, 0xffffffff, 28)));
            VERIFY_IS_TRUE(brightness(plate.seam) < brightness(plate.lowerSeam));
            VERIFY_IS_TRUE(brightness(plate.lowerSeam) < brightness(plate.face));
            VERIFY_IS_TRUE(brightness(plate.lettering) < brightness(plate.face));
            VERIFY_IS_TRUE(plate.face != previousFace); // Swatches remain casing-aware.
            previousFace = plate.face;
            for (const auto dpiScale : { 1.0f, 1.25f, 1.5f, 2.0f })
            {
                VERIFY_ARE_EQUAL(10.0f * dpiScale,
                                 (BezelBadgeSeam + BezelBadgeHorizontalContentInset - BezelBadgeSeam) * dpiScale);
                VERIFY_ARE_EQUAL(2.0f * dpiScale,
                                 (BezelBadgeSeam + BezelBadgeVerticalContentInset - BezelBadgeSeam) * dpiScale);
                VERIFY_IS_TRUE(BezelBadgeSeam * dpiScale <= 2.0f);
            }
        }
        VERIFY_ARE_EQUAL(std::uint16_t{ 700 }, BezelBadgeFontWeight);
        VERIFY_ARE_EQUAL(BezelPaletteArgb(0), BezelPaletteArgb(-1));
        VERIFY_ARE_EQUAL(BezelPaletteArgb(0), BezelPaletteArgb(BezelPaletteCount));
        VERIFY_ARE_EQUAL(GetBezelBadgeMaterial(0).face, GetBezelBadgeMaterial(BezelPaletteCount).face);
    }

    void ProfileSourceTests::BackslashBezelDarkPalettesContrastAndIsolation()
    {
        using namespace Microsoft::Terminal::Backslash;
        auto luminance = [](std::uint32_t argb) {
            auto linear = [](unsigned int value) {
                const auto srgb = value / 255.0;
                return srgb <= 0.04045 ? srgb / 12.92 : std::pow((srgb + 0.055) / 1.055, 2.4);
            };
            return 0.2126 * linear((argb >> 16) & 255u) + 0.7152 * linear((argb >> 8) & 255u) + 0.0722 * linear(argb & 255u);
        };
        constexpr std::uint32_t colors[] = { 0xffe8e8dd, 0xffe5dabd, 0xffd9c796, 0xffc4ab68,
                                             0xff666b6c, 0xff414648, 0xff222629, 0xff566572, 0xff626a55 };
        VERIFY_ARE_EQUAL(9, BezelPaletteCount);
        auto settings = winrt::make_self<implementation::CascadiaSettings>(localJSON);
        const auto profile = settings->AllProfiles().GetAt(0);
        const auto appearance = profile.DefaultAppearance();
        appearance.DarkColorSchemeName(L"Campbell");
        appearance.Opacity(0.4f);
        appearance.UseAcrylic(true);
        appearance.RetroTerminalGlowIntensity(0.3f);
        appearance.RetroTerminalScanlineIntensity(0.5f);
        for (int color = 0; color < BezelPaletteCount; ++color)
        {
            VERIFY_ARE_EQUAL(colors[color], BezelPaletteArgb(color));
            VERIFY_ARE_EQUAL(color >= 4, IsDarkBezelPalette(color));
            profile.RetroBezelColor(static_cast<RetroBezelColor>(color));
            VERIFY_ARE_EQUAL(L"Campbell", appearance.DarkColorSchemeName());
            VERIFY_ARE_EQUAL(0.4f, appearance.Opacity());
            VERIFY_IS_TRUE(appearance.UseAcrylic());
            VERIFY_ARE_EQUAL(0.3f, appearance.RetroTerminalGlowIntensity());
            VERIFY_ARE_EQUAL(0.5f, appearance.RetroTerminalScanlineIntensity());
            VERIFY_ARE_EQUAL(0xff000000u, BezelSurroundArgb);
            if (!IsDarkBezelPalette(color)) { continue; }
            const auto casing = GetDarkBezelCasingMaterial(color);
            const auto plate = GetBezelBadgeMaterial(color);
            for (const auto argb : { casing.top, casing.bottom, casing.rimTop, casing.rimBottom,
                                     casing.recessTop, casing.recessBottom, plate.face, plate.seam, plate.lowerSeam, plate.lettering })
            {
                VERIFY_ARE_EQUAL(255u, argb >> 24);
            }
            VERIFY_IS_TRUE(luminance(casing.top) > luminance(casing.bottom));
            VERIFY_IS_TRUE(luminance(casing.rimTop) > luminance(casing.top));
            VERIFY_IS_TRUE(luminance(casing.recessTop) < luminance(casing.bottom));
            VERIFY_IS_TRUE(luminance(casing.recessBottom) > luminance(BezelPaletteArgb(color)));
            VERIFY_IS_TRUE(luminance(plate.seam) < luminance(plate.face));
            VERIFY_IS_TRUE(luminance(plate.face) < luminance(plate.lowerSeam));
            VERIFY_IS_TRUE(luminance(plate.face) < luminance(BezelPaletteArgb(color)));
            const auto contrast = (luminance(plate.lettering) + 0.05) / (luminance(plate.face) + 0.05);
            VERIFY_IS_TRUE(contrast >= 4.5); // Readable small wordmark against the actual opaque plate.
        }
        VERIFY_IS_TRUE(luminance(BezelPaletteArgb(4)) > luminance(BezelPaletteArgb(5)));
        VERIFY_IS_TRUE(luminance(BezelPaletteArgb(5)) > luminance(BezelPaletteArgb(6)));
        VERIFY_IS_FALSE(IsDarkBezelPalette(-1));
        VERIFY_IS_FALSE(IsDarkBezelPalette(BezelPaletteCount));
    }

    void ProfileSourceTests::BackslashBezelRecessCornerRadii()
    {
        using namespace Microsoft::Terminal::Backslash;
        VERIFY_ARE_EQUAL(0.0f, BezelOuterRadius); // Casing silhouette remains square.
        VERIFY_ARE_EQUAL(6.0f, BezelRecessOuterRadius);
        VERIFY_ARE_EQUAL(16.0f, BezelInnerRadius);
        for (const auto dimensions : { std::pair{ 800.0f, 600.0f }, std::pair{ 120.0f, 20.0f },
                                       std::pair{ 1.0f, 1.0f }, std::pair{ 2.0f, 10.0f }, std::pair{ 10.0f, 2.0f } })
        {
            const auto [width, height] = dimensions;
            const auto radii = GetBezelRecessRadii(width, height);
            VERIFY_IS_TRUE(radii.outer > 0 && radii.outer < radii.inner);
            VERIFY_IS_TRUE(radii.inner * 2 <= width && radii.inner * 2 <= height);
            VERIFY_IS_TRUE(BezelOpeningContainsInsetCorner(radii.inner, BezelInnerClearance));
            VERIFY_ARE_EQUAL(radii.inner * BezelRecessOuterRadius / BezelInnerRadius, radii.outer);
            const auto rotated = GetBezelRecessRadii(height, width);
            VERIFY_ARE_EQUAL(radii.inner, rotated.inner);
            VERIFY_ARE_EQUAL(radii.outer, rotated.outer);
            for (const auto scale : { 1.0f, 1.25f, 1.5f, 2.0f })
            {
                VERIFY_IS_TRUE(radii.outer * scale < radii.inner * scale);
                VERIFY_IS_TRUE(radii.inner * 2 * scale <= width * scale);
            }
        }
        for (const auto dimensions : { std::pair{ 0.0f, 100.0f }, std::pair{ 100.0f, 0.0f }, std::pair{ -1.0f, 100.0f } })
        {
            const auto radii = GetBezelRecessRadii(dimensions.first, dimensions.second);
            VERIFY_ARE_EQUAL(0.0f, radii.inner);
            VERIFY_ARE_EQUAL(0.0f, radii.outer);
        }
        for (int size = 0; size < 3; ++size)
        {
            for (int badge = 0; badge < 3; ++badge)
            {
                const auto screen = GetBezelScreenBounds(800, 600, GetBezelInsetsForMode(true, size, badge, true, true));
                const auto radii = GetBezelRecessRadii(screen.right - screen.left + 2 * BezelInnerClearance,
                                                      screen.bottom - screen.top + 2 * BezelInnerClearance);
                VERIFY_ARE_EQUAL(6.0f, radii.outer);
                VERIFY_ARE_EQUAL(16.0f, radii.inner);
                VERIFY_IS_TRUE(screen.Contains(400, 300));
            }
        }
    }

    void ProfileSourceTests::BackslashBezelThickerPresetDpiLayout()
    {
        using namespace Microsoft::Terminal::Backslash;
        auto settings = winrt::make_self<implementation::CascadiaSettings>(localJSON);
        const auto profile = settings->AllProfiles().GetAt(0);
        const auto fontSize = profile.FontInfo().FontSize();
        constexpr float edges[] = { 64, 80, 96 };
        for (int size = 0; size < 3; ++size)
        {
            profile.RetroBezelSize(static_cast<RetroBezelSize>(size));
            VERIFY_ARE_EQUAL(fontSize, profile.FontInfo().FontSize());
            for (int badge = 0; badge < 3; ++badge)
            {
                const auto insets = GetBezelInsetsForMode(true, size, badge, true, true);
                VERIFY_ARE_EQUAL(edges[size], insets.left);
                VERIFY_ARE_EQUAL(edges[size], insets.right);
                VERIFY_IS_TRUE(insets.left - BezelInnerClearance - BezelRecessThickness - BezelOuterRimThickness >= 7);
                if (badge)
                {
                    const auto side = badge == 2 ? insets.top : insets.bottom;
                    const auto plateOffset = GetBezelBadgeOuterOffset(side);
                    VERIFY_IS_TRUE(plateOffset >= BezelOuterRimThickness);
                    VERIFY_IS_TRUE(plateOffset + BezelBadgeHeight <= side - BezelInnerClearance - BezelRecessThickness);
                }
                for (const auto scale : { 1.0f, 1.25f, 1.5f, 2.0f })
                {
                    VERIFY_ARE_EQUAL(edges[size] * scale, insets.left * scale);
                    VERIFY_ARE_EQUAL(edges[size] * 2 * scale, insets.Width() * scale);
                    const auto screen = GetBezelScreenBounds(800, 600, insets);
                    const auto radii = GetBezelRecessRadii(screen.right - screen.left + 12, screen.bottom - screen.top + 12);
                    VERIFY_IS_TRUE(radii.outer * scale < radii.inner * scale);
                }
            }
        }
    }

    void ProfileSourceTests::BackslashBezelSubstantialRecessGeometry()
    {
        using namespace Microsoft::Terminal::Backslash;
        VERIFY_ARE_EQUAL(18.0f, BezelRecessThickness);
        VERIFY_ARE_EQUAL(6.0f, BezelInnerClearance);
        constexpr float casingWidths[] = { 40, 56, 72 };
        constexpr float totalInsets[] = { 64, 80, 96 };
        for (int size = 0; size < 3; ++size)
        {
            VERIFY_ARE_EQUAL(casingWidths[size], GetVisibleBezelCasingWidth(size));
            for (int badge = 0; badge < 3; ++badge)
            {
                for (const auto fullscreen : { false, true })
                {
                    for (const auto center : { false, true })
                    {
                        const auto insets = GetBezelInsetsForMode(true, size, badge, center, fullscreen);
                        VERIFY_ARE_EQUAL(totalInsets[size], insets.left);
                        VERIFY_ARE_EQUAL(totalInsets[size], insets.right);
                        const auto bounds = GetBezelApertureBounds(1000, 800, insets);
                        VERIFY_ARE_EQUAL(casingWidths[size], bounds.recess.left);
                        VERIFY_ARE_EQUAL(casingWidths[size], 1000 - bounds.recess.right);
                        VERIFY_ARE_EQUAL(18.0f, bounds.opening.left - bounds.recess.left);
                        VERIFY_ARE_EQUAL(18.0f, bounds.opening.top - bounds.recess.top);
                        VERIFY_ARE_EQUAL(18.0f, bounds.recess.right - bounds.opening.right);
                        VERIFY_ARE_EQUAL(18.0f, bounds.recess.bottom - bounds.opening.bottom);
                        VERIFY_ARE_EQUAL(6.0f, bounds.screen.left - bounds.opening.left);
                        VERIFY_ARE_EQUAL(6.0f, bounds.screen.top - bounds.opening.top);
                        const auto radii = GetBezelRecessRadii(bounds.opening.right - bounds.opening.left,
                                                              bounds.opening.bottom - bounds.opening.top);
                        // All four screen corners lie inside their rounded arc with
                        // more than one DIP clearance, without clipping terminal cells.
                        const auto delta = radii.inner - BezelInnerClearance;
                        VERIFY_IS_TRUE(2 * delta * delta < (radii.inner - 1) * (radii.inner - 1));
                        for (const auto scale : { 1.0f, 1.25f, 1.5f, 2.0f })
                        {
                            VERIFY_ARE_EQUAL(18.0f * scale, (bounds.opening.left - bounds.recess.left) * scale);
                            VERIFY_ARE_EQUAL(casingWidths[size] * scale, bounds.recess.left * scale);
                            VERIFY_ARE_EQUAL(totalInsets[size] * scale, insets.left * scale);
                        }
                    }
                }
            }
            // A two-DIP screen keeps its rounded hole bounded and its cells safe.
            const auto insets = GetBezelInsets(true, size, 0);
            const auto minimum = GetBezelApertureBounds(insets.Width() + 2, insets.Height() + 2, insets);
            const auto radii = GetBezelRecessRadii(minimum.opening.right - minimum.opening.left,
                                                  minimum.opening.bottom - minimum.opening.top);
            VERIFY_ARE_EQUAL(7.0f, radii.inner);
            VERIFY_IS_TRUE(BezelOpeningContainsInsetCorner(radii.inner, BezelInnerClearance));
        }
        for (int color = 0; color < BezelPaletteCount; ++color)
        {
            const auto band = GetBezelRecessMaterial(color);
            VERIFY_ARE_EQUAL(255u, band.shadow >> 24);
            VERIFY_ARE_EQUAL(255u, band.highlight >> 24);
            VERIFY_IS_TRUE(band.shadow != band.highlight);
            if (!IsDarkBezelPalette(color))
            {
                // Broad light-plastic recess remains dark at both edges.
                for (const auto shift : { 0u, 8u, 16u })
                {
                    VERIFY_IS_TRUE(((band.shadow >> shift) & 255u) < ((band.highlight >> shift) & 255u));
                    VERIFY_IS_TRUE(((band.highlight >> shift) & 255u) < ((BezelPaletteArgb(color) >> shift) & 255u));
                }
            }
        }
    }

    void ProfileSourceTests::BackslashBezelAppearancePreviewViewportSizing()
    {
        using namespace Microsoft::Terminal::Backslash;
        for (const auto enabled : { false, true })
        {
            for (int size = 0; size < 3; ++size)
            {
                for (int badge = 0; badge < 3; ++badge)
                {
                    const auto bezel = GetBezelInsets(enabled, size, badge);
                    for (const auto available : { 0.0f, 120.0f, 240.0f, 400.0f, 640.0f, 1000.0f, 1600.0f })
                    {
                        for (const auto scale : { 1.0f, 1.25f, 1.5f, 2.0f })
                        {
                            const auto layout = GetAppearancePreviewLayout(available, bezel,
                                                                            (320 + bezel.Width()) * scale,
                                                                            (220 + bezel.Height()) * scale, scale);
                            VERIFY_IS_TRUE(layout.viewportWidth >= 320);
                            VERIFY_ARE_EQUAL(220.0f, layout.viewportHeight);
                            VERIFY_ARE_EQUAL(220 + bezel.Height() + 2, layout.height);
                            VERIFY_IS_TRUE(layout.width >= 320 + bezel.Width() + 2);
                            if (available > 0 && available < 320 + bezel.Width() + 2) { VERIFY_IS_TRUE(layout.requiresHorizontalScroll); }
                            if (available >= 320 + bezel.Width() + 2) { VERIFY_IS_FALSE(layout.requiresHorizontalScroll); }
                            VERIFY_IS_TRUE(layout.width <= 1000);
                        }
                    }
                    // Large fonts/padding reserve their whole sample, then scroll.
                    const auto large = GetAppearancePreviewLayout(400, bezel,
                                                                  (1200 + bezel.Width()) * 1.5f,
                                                                  (600 + bezel.Height()) * 1.5f, 1.5f);
                    VERIFY_ARE_EQUAL(1200.0f, large.viewportWidth);
                    VERIFY_ARE_EQUAL(600.0f, large.viewportHeight);
                    VERIFY_IS_TRUE(large.requiresHorizontalScroll);
                }
            }
        }
        const auto medium = GetBezelInsets(true, 1, 1);
        const auto defaultPreview = GetAppearancePreviewLayout(640, medium, 0, 0, 1);
        VERIFY_ARE_EQUAL(640.0f, defaultPreview.width);
        VERIFY_ARE_EQUAL(410.0f, defaultPreview.height); // Old 180 height lost its entire viewport.
        const auto nan = std::numeric_limits<float>::quiet_NaN();
        const auto invalid = GetAppearancePreviewLayout(nan, medium, nan, nan, nan);
        VERIFY_IS_TRUE(std::isfinite(invalid.width) && std::isfinite(invalid.height));
        VERIFY_IS_TRUE(invalid.viewportWidth >= 320 && invalid.viewportHeight >= 220);
    }

    void ProfileSourceTests::BackslashBezelAppearancePreviewDefaultsAndOverrides()
    {
        using namespace Microsoft::Terminal::Backslash;
        auto settings = winrt::make_self<implementation::CascadiaSettings>(R"({"profiles":{
            "defaults":{"experimental.retroBezelEnabled":true,"experimental.retroBezelSize":"wide","experimental.retroBezelBadge":"bottom"},
            "list":[{"name":"inherited","guid":"{6239a42c-1de4-49a3-80bd-e8fdd045185c}"},
                     {"name":"override","guid":"{6239a42c-1de4-49a3-80bd-e8fdd045185d}","experimental.retroBezelSize":"slim","experimental.retroBezelBadge":"top"}]}})");
        const auto defaults = settings->ProfileDefaults();
        const auto inherited = settings->AllProfiles().GetAt(0);
        const auto overridden = settings->AllProfiles().GetAt(1);
        auto sizeFor = [](const auto& profile) {
            const auto insets = GetBezelInsets(profile.RetroBezelEnabled(), static_cast<int>(profile.RetroBezelSize()),
                                              static_cast<int>(profile.RetroBezelBadge()));
            return GetAppearancePreviewLayout(600, insets, 0, 0, 1);
        };
        for (int size = 0; size < 3; ++size)
        {
            for (int badge = 0; badge < 3; ++badge)
            {
                defaults.RetroBezelSize(static_cast<RetroBezelSize>(size));
                defaults.RetroBezelBadge(static_cast<RetroBezelBadge>(badge));
                const auto base = sizeFor(defaults);
                const auto child = sizeFor(inherited);
                VERIFY_ARE_EQUAL(base.height, child.height);
                VERIFY_ARE_EQUAL(base.width, child.width);
                VERIFY_ARE_EQUAL(220.0f, child.viewportHeight);
                VERIFY_IS_TRUE(child.viewportWidth >= 320);
                const auto local = sizeFor(overridden);
                VERIFY_ARE_EQUAL(378.0f, local.height); // Slim + top chin + sample + stroke.
                VERIFY_ARE_EQUAL(220.0f, local.viewportHeight);
            }
        }
        defaults.RetroBezelEnabled(false);
        VERIFY_ARE_EQUAL(222.0f, sizeFor(inherited).height);
        overridden.RetroBezelEnabled(true);
        overridden.RetroBezelSize(RetroBezelSize::Wide);
        VERIFY_ARE_EQUAL(442.0f, sizeFor(overridden).height);
        overridden.ClearRetroBezelEnabled();
        VERIFY_ARE_EQUAL(222.0f, sizeFor(overridden).height);
    }

    void ProfileSourceTests::BackslashBezelDirectionalMaterialLighting()
    {
        using namespace Microsoft::Terminal::Backslash;
        VERIFY_ARE_EQUAL(0.0f, BezelCasingGradient.startX);
        VERIFY_ARE_EQUAL(0.5f, BezelCasingGradient.startY);
        VERIFY_ARE_EQUAL(1.0f, BezelCasingGradient.endX);
        VERIFY_ARE_EQUAL(0.5f, BezelCasingGradient.endY);
        VERIFY_ARE_EQUAL(0.0f, BezelRecessGradient.startX);
        VERIFY_ARE_EQUAL(1.0f, BezelRecessGradient.startY);
        VERIFY_ARE_EQUAL(1.0f, BezelRecessGradient.endX);
        VERIFY_ARE_EQUAL(0.0f, BezelRecessGradient.endY);
        auto parameter = [](BezelGradientDirection direction, float x, float y, float width, float height) {
            const auto dx = (direction.endX - direction.startX) * width;
            const auto dy = (direction.endY - direction.startY) * height;
            return ((x - direction.startX) * width * dx + (y - direction.startY) * height * dy) / (dx * dx + dy * dy);
        };
        for (const auto dimensions : { std::pair{ 800.0f, 600.0f }, std::pair{ 600.0f, 400.0f }, std::pair{ 400.0f, 1000.0f } })
        {
            const auto [width, height] = dimensions;
            VERIFY_ARE_EQUAL(0.0f, parameter(BezelCasingGradient, 0, 0.5f, width, height));
            VERIFY_ARE_EQUAL(1.0f, parameter(BezelCasingGradient, 1, 0.5f, width, height));
            // Recess endpoints are bright bottom-left and dark top-right.
            // Both left and bottom edges stay brighter than top and right.
            VERIFY_ARE_EQUAL(0.0f, parameter(BezelRecessGradient, 0, 1, width, height));
            VERIFY_ARE_EQUAL(1.0f, parameter(BezelRecessGradient, 1, 0, width, height));
            const auto left = parameter(BezelRecessGradient, 0, 0.5f, width, height);
            const auto bottom = parameter(BezelRecessGradient, 0.5f, 1, width, height);
            const auto right = parameter(BezelRecessGradient, 1, 0.5f, width, height);
            const auto top = parameter(BezelRecessGradient, 0.5f, 0, width, height);
            VERIFY_IS_TRUE(left < right && left < top && bottom < right && bottom < top);
        }
        auto brightness = [](std::uint32_t argb) { return (argb & 255u) + ((argb >> 8) & 255u) + ((argb >> 16) & 255u); };
        for (int color = 0; color < BezelPaletteCount; ++color)
        {
            const auto casing = GetBezelCasingMaterial(color);
            const auto recess = GetBezelRecessMaterial(color);
            VERIFY_IS_TRUE(brightness(casing.rightHighlight) > brightness(casing.leftShadow));
            VERIFY_IS_TRUE(brightness(recess.highlight) > brightness(recess.shadow));
            for (const auto value : { casing.leftShadow, casing.rightHighlight, recess.highlight, recess.shadow })
            {
                VERIFY_ARE_EQUAL(255u, value >> 24);
            }
        }
        VERIFY_ARE_EQUAL(18.0f, BezelRecessThickness);
        VERIFY_ARE_EQUAL(6.0f, BezelRecessOuterRadius);
        VERIFY_ARE_EQUAL(16.0f, BezelInnerRadius);
        VERIFY_ARE_EQUAL(0xff000000u, BezelSurroundArgb);
    }

    void ProfileSourceTests::BackslashModernWslDiscovery()
    {
        std::vector<winrt::com_ptr<implementation::Profile>> profiles;
        WslDistroGenerator::GenerateForDistributions({ { L"Ubuntu-24.04", true }, { L"Debian", false }, { L"docker-desktop", true }, { L"docker-desktop-data", false }, { L"rancher-desktop", true }, { L"Ubuntu-24.04", true } }, profiles);
        VERIFY_ARE_EQUAL(size_t{ 2 }, profiles.size());
        VERIFY_ARE_EQUAL(CreateDynamicProfile(L"Ubuntu-24.04")->Guid(), profiles[0]->Guid());
        VERIFY_ARE_EQUAL(CreateDynamicProfile(L"Debian")->Guid(), profiles[1]->Guid());
        std::wstring system;
        THROW_IF_FAILED(wil::GetSystemDirectoryW<std::wstring>(system));
        VERIFY_ARE_EQUAL(winrt::hstring{ system + L"\\wsl.exe -d Ubuntu-24.04" }, profiles[0]->Commandline());
        std::vector<winrt::com_ptr<implementation::Profile>> legacy;
        WslDistroGenerator::GenerateForDistributions({ { L"Ubuntu-24.04", false } }, legacy);
        VERIFY_ARE_EQUAL(profiles[0]->Guid(), legacy[0]->Guid());
        VERIFY_ARE_EQUAL(profiles[0]->Commandline(), legacy[0]->Commandline());
        // Real source filtering remains shared between normal startup and explicit discovery.
        class FixtureWsl : public IDynamicProfileGenerator
        {
            std::wstring_view GetNamespace() const noexcept override { return L"Windows.Terminal.Wsl"; }
            std::wstring_view GetDisplayName() const noexcept override { return L"WSL fixture"; }
            std::wstring_view GetIcon() const noexcept override { return {}; }
            void GenerateProfiles(std::vector<winrt::com_ptr<implementation::Profile>>& p) const override
            {
                WslDistroGenerator::GenerateForDistributions({ { L"Ubuntu-24.04", true } }, p);
            }
        } generator;
        implementation::SettingsLoader disabled{ R"({"disabledProfileSources":["Windows.Terminal.Wsl"]})", "{}" };
        VERIFY_ARE_EQUAL(size_t{ 0 }, disabled.GenerateProfilesFor(generator).size());
    }
}
