// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "pch.h"
#include "../inc/PaneProfilePolicy.h"
#include "../inc/FocusedPaneClose.h"
#include "../TerminalSettingsModel/GlobalAppSettings.h"
#include "../TerminalSettingsModel/SavedTabGroups.h"
#include "../TerminalApp/SettingsViewHost.h"
using namespace WEX::TestExecution;
namespace P = Microsoft::Terminal::Backslash::PaneProfilePolicy;
namespace B = Microsoft::Terminal::Backslash;
namespace M = winrt::Microsoft::Terminal::Settings::Model;
namespace SettingsModelUnitTests
{
    class PaneProfileTests
    {
        BEGIN_TEST_CLASS(PaneProfileTests)
        END_TEST_CLASS()
        TEST_METHOD(TerminalOnlyAndProtectedTargets);
        TEST_METHOD(CommitClosesOnlyOldContent);
        TEST_METHOD(AttachmentFailureRestoresOldContent);
        TEST_METHOD(ExchangeFailureDoesNotCloseAnything);
        TEST_METHOD(SuppressionRequiresAffirmativeValidCommit);
        TEST_METHOD(WarningPreferenceIsScopedAndRoundtrips);
        TEST_METHOD(GlobalDraftGuardCoversOtherWindows);
        TEST_METHOD(GroupCaptureUsesChosenIdentityAndGeometry);
        TEST_METHOD(RefreshFailureRestoresDisplayedState);
        TEST_METHOD(InactiveTabCannotTakeKeyboardOrWindowBackground);
    };
    void PaneProfileTests::TerminalOnlyAndProtectedTargets()
    {
        VERIFY_IS_TRUE(P::Eligible(true,false,false,false,false));
        VERIFY_IS_FALSE(P::Eligible(false,false,false,false,false)); // Settings, Transfers, native view or selected parent.
        VERIFY_IS_FALSE(P::Eligible(true,true,false,false,false));
        VERIFY_IS_FALSE(P::Eligible(true,false,true,false,false));
        VERIFY_IS_FALSE(P::Eligible(true,false,false,true,false));
        VERIFY_IS_FALSE(P::Eligible(true,false,false,false,true));
    }
    void PaneProfileTests::CommitClosesOnlyOldContent()
    {
        auto old = std::make_shared<int>(1), replacement = std::make_shared<int>(2), sibling = std::make_shared<int>(3);
        auto leaf = old; unsigned closed=0, wired=0, refreshed=0;
        P::Replace(old,replacement,[&](auto next) { return std::exchange(leaf,next); },
            [&](auto value) { VERIFY_IS_TRUE(value==replacement); ++wired; },[&] { ++refreshed; },
            [&](auto value) { VERIFY_IS_TRUE(value==old); ++closed; },[] {});
        VERIFY_IS_TRUE(leaf==replacement); VERIFY_ARE_EQUAL(3,*sibling);
        VERIFY_ARE_EQUAL(1u,closed); VERIFY_ARE_EQUAL(1u,wired); VERIFY_ARE_EQUAL(1u,refreshed);
    }
    void PaneProfileTests::AttachmentFailureRestoresOldContent()
    {
        auto old=std::make_shared<int>(1), replacement=std::make_shared<int>(2), leaf=old;
        unsigned closed=0, restored=0;
        const auto operation=[&] { P::Replace(old,replacement,[&](auto next) { return std::exchange(leaf,next); },
            [&](auto value) { if(value==replacement) { throw std::runtime_error("synthetic binding failure"); } ++restored; },
            [] {},[&](auto) { ++closed; },[] {}); };
        VERIFY_THROWS(operation(),std::runtime_error); VERIFY_IS_TRUE(leaf==old); VERIFY_ARE_EQUAL(0u,closed); VERIFY_ARE_EQUAL(1u,restored);
    }
    void PaneProfileTests::ExchangeFailureDoesNotCloseAnything()
    {
        auto old=std::make_shared<int>(1), replacement=std::make_shared<int>(2);
        unsigned closed=0;
        const auto operation=[&] { P::Replace(old,replacement,[](auto) -> std::shared_ptr<int> { throw std::runtime_error("synthetic construction failure"); },
            [&](auto value) { VERIFY_IS_TRUE(value==old); },[] {},[&](auto) { ++closed; },[] {}); };
        VERIFY_THROWS(operation(),std::runtime_error); VERIFY_ARE_EQUAL(0u,closed);
    }
    void PaneProfileTests::SuppressionRequiresAffirmativeValidCommit()
    {
        VERIFY_IS_TRUE(P::SaveSuppression(true,true,true,true));
        VERIFY_IS_FALSE(P::SaveSuppression(false,true,true,true));
        VERIFY_IS_FALSE(P::SaveSuppression(true,false,true,true));
        VERIFY_IS_FALSE(P::SaveSuppression(true,true,false,true));
        VERIFY_IS_FALSE(P::SaveSuppression(true,true,true,false));
    }
    void PaneProfileTests::RefreshFailureRestoresDisplayedState()
    {
        auto old=std::make_shared<int>(1), replacement=std::make_shared<int>(2), leaf=old;
        std::string title="old", progress="old-progress", background="old-background";
        std::vector<int> mru{1,3}; unsigned closed=0;
        const auto operation=[&] { P::Replace(old,replacement,[&](auto next) { return std::exchange(leaf,next); },
            [](auto) {},[&] { title="new"; progress="new-progress"; background="new-background";mru={2,3};throw std::runtime_error("partial refresh"); },
            [&](auto) { ++closed; },[&] { title="old"; progress="old-progress"; background="old-background";mru={1,3}; }); };
        VERIFY_THROWS(operation(),std::runtime_error); VERIFY_IS_TRUE(leaf==old); VERIFY_ARE_EQUAL(0u,closed);
        VERIFY_ARE_EQUAL(std::string{"old"},title); VERIFY_ARE_EQUAL(std::string{"old-progress"},progress);
        VERIFY_ARE_EQUAL(std::string{"old-background"},background); VERIFY_IS_TRUE(mru==std::vector<int>({1,3}));
    }
    void PaneProfileTests::InactiveTabCannotTakeKeyboardOrWindowBackground()
    {
        VERIFY_IS_TRUE(P::RestoreKeyboardFocus(true,true));
        VERIFY_IS_FALSE(P::RestoreKeyboardFocus(false,true)); VERIFY_IS_FALSE(P::RestoreKeyboardFocus(true,false));
        std::string windowBackground="selected";
        if(P::UpdateWindowAppearance(false)) { windowBackground="inactive"; }
        VERIFY_ARE_EQUAL(std::string{"selected"},windowBackground);
        if(P::UpdateWindowAppearance(true)) { windowBackground="new-selected"; }
        VERIFY_ARE_EQUAL(std::string{"new-selected"},windowBackground);
    }
    void PaneProfileTests::WarningPreferenceIsScopedAndRoundtrips()
    {
        auto settings=winrt::make_self<M::implementation::GlobalAppSettings>();
        VERIFY_IS_TRUE(settings->ConfirmPaneRelaunch());
        settings->ConfirmPaneRelaunch(false); VERIFY_IS_TRUE(settings->ConfirmCloseAllTabs());
        const auto copy=M::implementation::GlobalAppSettings::FromJson(settings->ToJson());
        VERIFY_IS_FALSE(copy->ConfirmPaneRelaunch()); VERIFY_IS_FALSE(copy->Copy()->ConfirmPaneRelaunch());
        copy->ConfirmPaneRelaunch(true); VERIFY_IS_TRUE(copy->ConfirmPaneRelaunch());
    }
    void PaneProfileTests::GlobalDraftGuardCoversOtherWindows()
    {
        winrt::TerminalApp::implementation::SettingsViewHost host;
        VERIFY_IS_TRUE(host.CanWritePreference());
        const auto lease=host.Request(); VERIFY_IS_FALSE(host.CanWritePreference());
        host.Ready(lease,[] {}); bool clean=false;
        host.PreferenceWriteGuard([&] { return clean; }); VERIFY_IS_FALSE(host.CanWritePreference());
        clean=true; VERIFY_IS_TRUE(host.CanWritePreference()); host.Release(lease);
    }
    void PaneProfileTests::GroupCaptureUsesChosenIdentityAndGeometry()
    {
        const wchar_t* id=L"{6B2A81D4-6A61-48BC-A675-EBB4A9706001}";
        M::NewTerminalArgs args; args.Profile(id);
        VERIFY_IS_TRUE(args.Commandline().empty()); VERIFY_IS_TRUE(args.StartingDirectory().empty());
        const auto tab=B::SavedTabGroups::CaptureTab({M::ActionAndArgs{M::ShortcutAction::NewTab,M::NewTabArgs{args}},
            M::ActionAndArgs{M::ShortcutAction::SplitPane,M::SplitPaneArgs{M::SplitType::Manual,M::SplitDirection::Right,0.35f,args}}},
            [](const auto&) { return winrt::hstring{L"Backslash.Terminal.SSH"}; });
        VERIFY_ARE_EQUAL(std::string{"Backslash.Terminal.SSH"},tab[1]["source"].asString());
        VERIFY_ARE_EQUAL(std::string{"right"},tab[1]["direction"].asString());
        VERIFY_ARE_EQUAL(0.35f,tab[1]["size"].asFloat());
        B::SavedTabGroup group; group.id="{5b2a81d4-6a61-48bc-a675-ebb4a9706001}";group.name="mixed";group.tabs.push_back(tab);
        VERIFY_THROWS(B::SavedTabGroups::Build(group,[](auto&&,auto&&) { return false; },[](auto&&) { return true; }),std::invalid_argument);
    }
}
