// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "pch.h"
#include "TerminalPage.h"
#include "TabRowControl.h"
#include "Remoting.h"
#include "../TerminalSettingsModel/PinnedLaunch.h"
#include "../TerminalSettingsModel/PinnedCapture.h"
#include "../TerminalSettingsModel/PinnedIdentity.h"
#include <filesystem>

using namespace winrt;
using namespace winrt::Microsoft::Terminal::Settings::Model;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
namespace P = ::Microsoft::Terminal::Backslash::Pins;
namespace C = ::Microsoft::Terminal::Backslash::ConnectionIdentityTitle;

namespace
{
    hstring PinFailure()
    {
        try { throw; }
        catch (const std::exception& e) { return to_hstring(e.what()); }
        catch (const hresult_error& e) { return e.message(); }
        catch (...) { return L"The pin operation failed. The remembered destination was preserved."; }
    }
    std::string ExpandPinLocalDirectory(const std::string& path)
    {
        const auto wide = to_hstring(path);
        return to_string(wil::ExpandEnvironmentStringsW<std::wstring>(wide.c_str()));
    }
    NewTerminalArgs TerminalArgs(const ActionAndArgs& action)
    {
        if (const auto args = action.Args().try_as<NewTabArgs>()) { return args.ContentArgs().try_as<NewTerminalArgs>(); }
        if (const auto args = action.Args().try_as<SplitPaneArgs>()) { return args.ContentArgs().try_as<NewTerminalArgs>(); }
        return nullptr;
    }
    bool HasContents(const IVector<ActionAndArgs>& actions, const winrt::TerminalApp::ContentManager& manager)
    {
        bool any = false;
        for (const auto& action : actions)
        {
            if (const auto args = TerminalArgs(action))
            {
                if (args.ContentId() == 0 || !manager.TryLookupCore(args.ContentId())) { return false; }
                any = true;
            }
        }
        return any;
    }
    safe_void_coroutine ReleaseWindow(std::shared_ptr<P::Store> store, P::Owner owner)
    {
        co_await resume_background();
        try { store->ReleaseWindow(owner); } CATCH_LOG()
    }
}

namespace winrt::TerminalApp::implementation
{
    void TerminalPage::SetPinStartupContext(bool collect, bool restoring)
    {
        _pinCollectEligible = collect; _pinRestoring = restoring;
    }
    void TerminalPage::_EnsurePins()
    {
        if (_pinStore) { return; }
        _pinOwner = { P::NewId(), GetCurrentProcessId(), P::ProcessStarted(GetCurrentProcess()),
            _WindowProperties.WindowId(), reinterpret_cast<uint64_t>(_hostingHwnd.value_or(nullptr)) };
        _pinLifetime.reset(CreateEventW(nullptr, TRUE, FALSE, P::LifetimeName(_pinOwner).c_str()));
        THROW_LAST_ERROR_IF(!_pinLifetime);
        _pinStore = std::make_shared<P::Store>(std::filesystem::path{ CascadiaSettings::SettingsDirectory().c_str() });
    }
    void TerminalPage::_PinError(const hstring& message)
    {
        if (!_pinsClosing) { _ShowControlNoticeDialog(RS_(L"Pins_ButtonLabel/Text"), message); }
    }
    com_ptr<Tab> TerminalPage::_FindPinTab(const std::string& id) const
    {
        for (const auto& tab : _tabs) { auto t = _GetTabImpl(tab); if (t->Pin && t->Pin->id == id) { return t; } }
        return nullptr;
    }
    uint32_t TerminalPage::_PinCount() const
    {
        uint32_t count = 0;
        for (const auto& tab : _tabs) { if (_GetTabImpl(tab)->Pin) { ++count; } }
        return count;
    }
    void TerminalPage::_NormalizePinOrder()
    {
        if (_pinNormalizing || _pinsClosing) { return; }
        _pinNormalizing = true;
        auto done = wil::scope_exit([&] { _pinNormalizing = false; });
        const auto selected = _tabView.SelectedItem();
        uint32_t destination = 0;
        std::vector<TerminalApp::Tab> pins;
        for (const auto& tab : _tabs) { if (_GetTabImpl(tab)->Pin) { pins.push_back(tab); } }
        for (const auto& tab : pins)
        {
            uint32_t index = 0;
            if (_tabs.IndexOf(tab, index) && index != destination) { _TryMoveTab(index, destination); }
            ++destination;
        }
        if (selected) { _tabView.SelectedItem(selected); }
    }
    TerminalApp::Tab TerminalPage::_AddPinEntry(PinTabView view, bool activate, uint32_t index)
    {
        if (view.icon.empty()) { view.icon = L"\xE756"; }
        view.placeholder = make_self<PinnedPaneContent>(view.destination ? to_hstring(P::Identity(*view.destination)) : RS_(L"Pins_Loading"), view.icon);
        view.placeholder->Message(view.busy ? RS_(L"Pins_Loading") : RS_(L"Pins_Dormant"));
        auto pane = std::make_shared<Pane>(view.placeholder.as<TerminalApp::IPaneContent>(), true);
        _nextPinView = std::move(view);
        return _CreateNewTabFromPane(std::move(pane), std::min(index, _PinCount()), !activate);
    }
    Profile TerminalPage::_PinProfile(const P::Destination& destination) const
    {
        for (const auto& profile : _settings.ActiveProfiles())
        {
            if (to_string(::Microsoft::Console::Utils::GuidToString(profile.Guid())) == destination.profile && to_string(profile.Source()) == destination.source) { return profile; }
        }
        throw std::invalid_argument("The pinned profile is unavailable or replaced. No default profile is substituted.");
    }
    P::ProfileTarget TerminalPage::_PinTarget(const Profile& profile) const
    {
        return P::TargetForProfile(profile, IsRunningElevated());
    }
    void TerminalPage::_StartPins()
    {
        if (_pinCollectEligible && !_pinsCollected && !_pinsClosing && _pinActionDepth == 0 && _tabs.Size())
        {
            _pinsCollected = true; _CollectPins();
        }
    }
    safe_void_coroutine TerminalPage::_CollectPins()
    {
        const auto strong = get_strong(); const auto dispatcher = Dispatcher();
        hstring error; std::vector<std::pair<P::Destination, P::Lease>> pins;
        try { _EnsurePins(); } catch (...) { _PinError(PinFailure()); co_return; }
        const auto store = _pinStore; const auto owner = _pinOwner;
        co_await resume_background();
        try { pins = store->ClaimUnviewed(owner); } catch (...) { error = PinFailure(); }
        co_await wil::resume_foreground(dispatcher);
        if (_pinsClosing) { ReleaseWindow(store, owner); co_return; }
        if (!error.empty()) { _PinError(error); co_return; }
        for (const auto& [pin, lease] : pins)
        {
            if (_FindPinTab(pin.id)) { continue; }
            PinTabView view; view.id = pin.id; view.destination = pin; view.lease = lease;
            try { view.icon = _PinProfile(pin).Icon().Resolved(); } CATCH_LOG()
            _AddPinEntry(std::move(view), false);
        }
    }
    void TerminalPage::_MaybeActivatePin()
    {
        if (_pinsClosing || _pinActionDepth || _pinNormalizing) { return; }
        if (const auto tab = _GetFocusedTabImpl())
        {
            if (tab->Pin && tab->Pin->dormant && !tab->Pin->busy && tab->Pin->destination && tab->Pin->lease) { _ActivatePin(*tab); }
        }
    }
    IAsyncOperation<hstring> TerminalPage::_ConfirmPinDirectory(P::Kind kind, hstring suggestion, bool invalidReport)
    {
        const auto strong = get_strong();
        _pinsFlyout.Hide();
        co_await wil::resume_foreground(Dispatcher(), Windows::UI::Core::CoreDispatcherPriority::Low);
        if (_pinsClosing) { co_return hstring{}; }
        namespace MUX = winrt::Microsoft::UI::Xaml;
        MUX::Controls::TeachingTip tip; tip.Title(RS_(L"Pins_ConfirmFolderTitle"));
        tip.IsLightDismissEnabled(false); tip.PreferredPlacement(MUX::Controls::TeachingTipPlacementMode::Center);
        tip.TailVisibility(MUX::Controls::TeachingTipTailVisibility::Collapsed);
        StackPanel form; form.Spacing(8); form.MaxWidth(480); form.TabFocusNavigation(Input::KeyboardNavigationMode::Cycle);
        TextBlock help; help.Text(RS_(kind == P::Kind::Local ? (invalidReport ? L"Pins_ConfirmLocalInvalidReport" : L"Pins_ConfirmLocalFolder") : L"Pins_ConfirmRemoteFolder")); help.TextWrapping(TextWrapping::Wrap);
        TextBox path; path.Header(box_value(RS_(L"Pins_FolderToPin"))); path.Text(suggestion); path.MaxLength(32760);
        path.ContextFlyout(winrt::Microsoft::Terminal::UI::TextMenuFlyout{});
        Automation::AutomationProperties::SetName(path, RS_(L"Pins_FolderToPin"));
        TextBlock resolved; resolved.TextWrapping(TextWrapping::Wrap);
        TextBlock error; error.TextWrapping(TextWrapping::Wrap); error.IsTextSelectionEnabled(true);
        StackPanel buttons; buttons.Orientation(Orientation::Horizontal); buttons.Spacing(8);
        Button pin; pin.Content(box_value(RS_(L"Pins_ConfirmPin")));
        Button cancel; cancel.Content(box_value(RS_(L"Pins_Cancel")));
        buttons.Children().Append(pin); buttons.Children().Append(cancel);
        form.Children().Append(help); form.Children().Append(path); form.Children().Append(resolved); form.Children().Append(error); form.Children().Append(buttons);
        tip.Content(form); Root().Children().Append(tip); _UpdateTeachingTipTheme(tip);
        auto remove = wil::scope_exit([&] { uint32_t index = 0; if (Root().Children().IndexOf(tip, index)) { Root().Children().RemoveAt(index); } });
        hstring selected; hstring preview; bool accepted = false, unloaded = false, pressedEnter = false; int layouts = 0;
        const auto validate = [&] {
            try
            {
                const auto directory = P::ConfirmDirectory(kind, to_string(path.Text()), ExpandPinLocalDirectory);
                preview = to_hstring(directory.path); resolved.Text(RS_(L"Pins_ResolvedFolder") + preview); error.Text({}); pin.IsEnabled(true);
            }
            catch (...) { preview = {}; resolved.Text({}); error.Text(PinFailure()); pin.IsEnabled(false); }
        };
        validate();
        const auto change = path.TextChanged(winrt::auto_revoke, [&](auto&&, auto&&) { validate(); });
        TextBox::LayoutUpdated_revoker focus;
        focus = path.LayoutUpdated(winrt::auto_revoke, [&](auto&&, auto&&) { if (++layouts >= 2 && tip.IsOpen() && path.Focus(FocusState::Programmatic)) { path.SelectAll(); focus.revoke(); } });
        wil::unique_event closed; closed.create(wil::EventOptions::ManualReset);
        const auto go = pin.Click(winrt::auto_revoke, [&](auto&&, auto&&) { if (pin.IsEnabled()) { selected = preview; accepted = true; tip.IsOpen(false); } });
        const auto stop = cancel.Click(winrt::auto_revoke, [&](auto&&, auto&&) { tip.IsOpen(false); });
        const auto down = form.KeyDown(winrt::auto_revoke, [&](auto&&, const Input::KeyRoutedEventArgs& e) { if (e.OriginalKey() == Windows::System::VirtualKey::Enter) { pressedEnter = true; e.Handled(true); } else if (e.OriginalKey() == Windows::System::VirtualKey::Escape) { e.Handled(true); } });
        const auto up = form.KeyUp(winrt::auto_revoke, [&](auto&&, const Input::KeyRoutedEventArgs& e) { if (e.OriginalKey() == Windows::System::VirtualKey::Enter) { e.Handled(true); if (pressedEnter && pin.IsEnabled()) { selected = preview; accepted = true; tip.IsOpen(false); } pressedEnter = false; } else if (e.OriginalKey() == Windows::System::VirtualKey::Escape) { e.Handled(true); tip.IsOpen(false); } });
        const auto onClosed = tip.Closed(winrt::auto_revoke, [&](auto&&, auto&&) { closed.SetEvent(); });
        const auto onUnload = Unloaded(winrt::auto_revoke, [&](auto&&, auto&&) { unloaded = true; accepted = false; tip.IsOpen(false); closed.SetEvent(); });
        tip.IsOpen(true); co_await resume_on_signal(closed.get()); co_await wil::resume_foreground(Dispatcher()); focus.revoke();
        if (unloaded || _pinsClosing) { co_return hstring{}; }
        _FocusCurrentTab(false);
        co_return accepted ? selected : hstring{};
    }

    safe_void_coroutine TerminalPage::_PinFocusedDestination(bool manual)
    {
        const auto strong = get_strong(); const auto dispatcher = Dispatcher();
        if (_pinCaptureActive || _pinsClosing) { co_return; }
        _pinCaptureActive = true; auto done = wil::scope_exit([&] { _pinCaptureActive = false; });
        P::Destination pin; P::DirectoryRequest request; hstring icon; com_ptr<Tab> sourceTab;
        try
        {
            _EnsurePins(); sourceTab = _GetFocusedTabImpl();
            if (!sourceTab) { throw std::invalid_argument("Select a terminal pane first."); }
            const auto content = sourceTab->GetActiveContent().try_as<TerminalApp::TerminalPaneContent>();
            if (!content) { throw std::invalid_argument("Select a terminal pane first."); }
            const auto profile = content.GetProfile(); const auto control = content.GetTermControl(); auto target = _PinTarget(profile);
            if (!target.supportsFixedPath) { throw std::invalid_argument(profile.Source() == L"Windows.Terminal.SSH" ? "Fixed SSH folders currently require a generated SSH profile with PowerShell 7 selected." : "This profile's launcher cannot safely reopen a pinned folder."); }
            request = P::PrepareCapture(target, P::InitialIdentity(control), to_string(control.ReportedWorkingDirectory()),
                target.kind == P::Kind::Local ? to_string(control.Settings().StartingDirectory()) : std::string{},
                target.kind == P::Kind::Local ? "%USERPROFILE%" : std::string{}, ExpandPinLocalDirectory);
            pin = { P::NewId(), target.profile, target.source, request.kind, to_string(profile.Name()), target.host, request.path };
            icon = profile.Icon().Resolved();
            if (!manual && request.needsConfirmation) { _PinError(RS_(L"Pins_CurrentUnavailable")); co_return; }
            if (!manual) { (void)P::CurrentDirectory(request); }
            if (manual) { request.needsConfirmation = true; }
        }
        catch (...) { _PinError(PinFailure()); co_return; }
        std::optional<std::string> confirmed;
        if (request.needsConfirmation)
        {
            const auto chosen = co_await _ConfirmPinDirectory(request.kind, to_hstring(request.path), request.invalidReport);
            if (chosen.empty() || _pinsClosing) { co_return; }
            confirmed = to_string(chosen);
        }
        try
        {
            uint32_t sourceIndex = 0;
            if (!_tabs.IndexOf(*sourceTab, sourceIndex)) { throw std::invalid_argument("The original tab was closed. Select a terminal pane and try again."); }
            const auto directory = P::FinishDirectory(request, confirmed, ExpandPinLocalDirectory);
            if (!directory) { co_return; }
            pin.kind = directory->kind; pin.path = directory->path; pin = P::Capture(std::move(pin), directory->origin);
            const auto current = _PinProfile(pin); auto target = _PinTarget(current); if (target.kind == P::Kind::SshWindows) { target.kind = pin.kind; }
            P::Resolve(pin, target, [](const auto&) { return true; });
            (void)P::PrepareLaunch(pin, current.Commandline(), current.SshShell(), current.SshBaseCommandline());
        }
        catch (...) { _PinError(PinFailure()); co_return; }
        const auto store = _pinStore; const auto owner = _pinOwner; std::optional<P::Lease> lease; hstring error;
        co_await resume_background();
        try
        {
            std::error_code directoryError;
            if (pin.kind == P::Kind::Local && !std::filesystem::is_directory(std::filesystem::path{ to_hstring(pin.path).c_str() }, directoryError)) { throw std::invalid_argument("Backslash couldn't open that folder. Check that it exists and that this session can access it."); }
            lease = store->Add(pin, owner);
        }
        catch (...) { error = PinFailure(); }
        co_await wil::resume_foreground(dispatcher);
        if (_pinsClosing) { ReleaseWindow(store, owner); co_return; }
        if (!error.empty()) { _PinError(error); co_return; }
        PinTabView view; view.id = pin.id; view.destination = pin; view.lease = lease; view.icon = icon;
        _AddPinEntry(std::move(view), false);
    }

    safe_void_coroutine TerminalPage::_OpenPinHere(std::string id, bool activate)
    {
        const auto strong = get_strong(); const auto dispatcher = Dispatcher();
        if (const auto tab = _FindPinTab(id))
        {
            if (activate) { _tabView.SelectedItem(tab->TabViewItem()); }
            if (!tab->Pin->busy && !tab->Pin->lease)
            {
                tab->PinBusy(true); tab->Pin->transfer.clear();
                NewTerminalArgs reference; reference.PinnedId(to_hstring(id));
                const auto actions = single_threaded_vector<ActionAndArgs>({ ActionAndArgs{ ShortcutAction::NewTab, NewTabArgs{ reference } } });
                _FinishPinReference(*tab, ActionAndArgs::Serialize(actions), true);
            }
            if (activate) { _MaybeActivatePin(); }
            co_return;
        }
        try { _EnsurePins(); } catch (...) { _PinError(PinFailure()); co_return; }
        const auto store = _pinStore; const auto owner = _pinOwner;
        std::optional<P::Lease> lease; std::optional<P::Destination> destination; hstring error;
        co_await resume_background();
        try { lease = store->Claim(id, owner); if (!lease) { throw std::runtime_error("This pin is already open or moving. Refresh the Pins menu."); } const auto state = store->Get(); for (const auto& pin : state.catalog.pins) { if (pin.id == id) { destination = pin; } } }
        catch (...) { error = PinFailure(); }
        co_await wil::resume_foreground(dispatcher);
        if (_pinsClosing) { ReleaseWindow(store, owner); co_return; }
        if (!error.empty()) { _PinError(error); co_return; }
        if (!destination || _FindPinTab(id)) { co_return; }
        PinTabView view; view.id = id; view.destination = destination; view.lease = lease;
        try { view.icon = _PinProfile(*destination).Icon().Resolved(); } CATCH_LOG()
        _AddPinEntry(std::move(view), activate);
        if (activate) { _MaybeActivatePin(); }
    }
    safe_void_coroutine TerminalPage::_ActivatePin(TerminalApp::Tab tab)
    {
        const auto strong = get_strong(); const auto dispatcher = Dispatcher(); const auto t = _GetTabImpl(tab);
        if (!t->Pin || !t->Pin->destination || !t->Pin->lease || t->Pin->busy || !t->Pin->dormant) { co_return; }
        const auto pin = *t->Pin->destination; const auto lease = *t->Pin->lease;
        t->PinBusy(true); hstring error; P::Launch launch;
        try { auto target = _PinTarget(_PinProfile(pin)); if (target.kind == P::Kind::SshWindows) { target.kind = pin.kind; } P::Resolve(pin, target, [](const auto&) { return true; }); launch = P::PrepareLaunch(pin, _PinProfile(pin).Commandline(), _PinProfile(pin).SshShell(), _PinProfile(pin).SshBaseCommandline()); }
        catch (...) { error = PinFailure(); }
        const auto store = _pinStore;
        co_await resume_background();
        if (error.empty())
        {
            try { const auto state = store->Get(); const auto it = state.leases.find(pin.id); if (it == state.leases.end() || it->second.owner != lease.owner || it->second.nonce != lease.nonce || !it->second.transfer.empty()) { throw std::runtime_error("The pin changed windows. Refresh the Pins menu."); } if (pin.kind == P::Kind::Local && !std::filesystem::is_directory(std::filesystem::path{ to_hstring(pin.path).c_str() })) { throw std::invalid_argument("The pinned folder is unavailable. Its destination was preserved."); } }
            catch (...) { error = PinFailure(); }
        }
        co_await wil::resume_foreground(dispatcher);
        uint32_t index = 0;
        if (_pinsClosing || !_tabs.IndexOf(tab, index) || !t->Pin) { co_return; }
        if (!error.empty()) { t->PinBusy(false); t->Pin->placeholder->Message(error); co_return; }
        try
        {
            // Revalidate profile identity after asynchronous work. Never substitute defaults.
            const auto profile = _PinProfile(pin); auto target = _PinTarget(profile); if (target.kind == P::Kind::SshWindows) { target.kind = pin.kind; }
            P::Resolve(pin, target, [](const auto&) { return true; }); launch = P::PrepareLaunch(pin, profile.Commandline(), profile.SshShell(), profile.SshBaseCommandline());
            NewTerminalArgs args; args.Profile(to_hstring(pin.profile)); args.StartingDirectory(hstring{ launch.directory }); args.Elevate(false);
            if (!launch.commandline.empty()) { args.Commandline(hstring{ launch.commandline }); }
            const auto pane = _MakePane(args, nullptr);
            if (!pane) { throw std::runtime_error("The pinned session could not be started."); }
            const auto selected = _tabView.SelectedItem(); const bool active = selected == tab.TabViewItem();
            auto view = *t->Pin; view.dormant = false; view.busy = false; view.placeholder = nullptr;
            t->SetPin({}); _nextPinView = std::move(view);
            const auto created = _CreateNewTabFromPane(pane, index, !active);
            _RemoveTab(tab);
            if (!active && selected) { _tabView.SelectedItem(selected); }
            if (active) { _tabView.SelectedItem(created.TabViewItem()); }
        }
        catch (...) { error = PinFailure(); if (t->Pin) { t->PinBusy(false); if (t->Pin->placeholder) { t->Pin->placeholder->Message(error); } } _nextPinView.reset(); _PinError(error); }
    }
    IAsyncOperation<bool> TerminalPage::_RemovePinPreference(TerminalApp::Tab tab)
    {
        const auto strong = get_strong(); const auto dispatcher = Dispatcher(); const auto t = _GetTabImpl(tab);
        if (!t->Pin || !t->Pin->lease) { co_return true; }
        const auto lease = *t->Pin->lease; const auto store = _pinStore;
        t->PinBusy(true); hstring error;
        co_await resume_background();
        try { store->Remove(lease.pin, lease.owner, lease.nonce); } catch (...) { error = PinFailure(); }
        co_await wil::resume_foreground(dispatcher);
        if (t->Pin) { t->PinBusy(false); }
        if (!error.empty()) { _PinError(error); co_return false; }
        co_return !_pinsClosing;
    }
    safe_void_coroutine TerminalPage::_Unpin(std::string id)
    {
        const auto strong = get_strong(); const auto t = _FindPinTab(id);
        if (!t || !t->Pin || t->Pin->dormant || t->Pin->busy) { co_return; }
        if (!(co_await _RemovePinPreference(*t))) { co_return; }
        uint32_t index = 0; if (!_tabs.IndexOf(*t, index)) { co_return; }
        t->SetPin({}); _TryMoveTab(index, _PinCount()); _NormalizePinOrder();
    }
    safe_void_coroutine TerminalPage::_RemoveClosedPin(std::string id)
    {
        const auto strong = get_strong(); const auto dispatcher = Dispatcher(); const auto store = _pinStore; hstring error;
        co_await resume_background(); try { store->RemoveUnviewed(id); } catch (...) { error = PinFailure(); }
        co_await wil::resume_foreground(dispatcher); if (!error.empty()) { _PinError(error); }
    }
    void TerminalPage::_ReleasePin(const P::Lease& lease)
    {
        const auto release = [](std::shared_ptr<P::Store> store, P::Lease copy) -> safe_void_coroutine {
            co_await resume_background();
            try { if (!copy.transfer.empty()) { store->CompleteTransfer(copy.pin, copy.transfer, copy.owner, copy.nonce); } store->Release(copy.pin, copy.owner, copy.nonce); } CATCH_LOG()
        };
        release(_pinStore, lease);
    }
    void TerminalPage::ShutdownPinnedTabs()
    {
        if (_pinsClosing) { return; } _pinsClosing = true;
        std::vector<TerminalApp::Tab> tabs; for (const auto& tab : _tabs) { tabs.push_back(tab); }
        for (const auto& tab : tabs) { const auto pin = _GetTabImpl(tab)->Pin; tab.Shutdown(); if (pin && pin->lease && !pin->lease->transfer.empty()) { _ReleasePin(*pin->lease); } }
        _pinLifetime.reset();
        if (_pinStore) { ReleaseWindow(_pinStore, _pinOwner); }
    }
    void TerminalPage::_CreatePinsFlyout()
    {
        _pinsFlyout = MenuFlyout{};
        _pinsFlyout.Opening([weak = get_weak()](auto&&, auto&&) { if (const auto self = weak.get()) { self->_RefreshPinsMenu(); } });
        get_self<TabRowControl>(_tabRow)->PinsButton().Flyout(_pinsFlyout);
    }
    safe_void_coroutine TerminalPage::_RefreshPinsMenu()
    {
        const auto strong = get_strong(); const auto dispatcher = Dispatcher(); P::Snapshot state; hstring error;
        try { _EnsurePins(); } catch (...) { _PinError(PinFailure()); co_return; }
        const auto store = _pinStore; _pinsFlyout.Items().Clear(); MenuFlyoutItem loading; loading.Text(RS_(L"Pins_Loading")); loading.IsEnabled(false); _pinsFlyout.Items().Append(loading);
        co_await resume_background(); try { state = store->Get(); } catch (...) { error = PinFailure(); }
        co_await wil::resume_foreground(dispatcher); if (_pinsClosing) { co_return; }
        auto items = _pinsFlyout.Items(); items.Clear();
        MenuFlyoutItem pin; pin.Text(RS_(L"Pins_Current")); pin.Click([weak = get_weak()](auto&&, auto&&) { if (auto self = weak.get()) { self->_PinFocusedDestination(); } }); items.Append(pin);
        MenuFlyoutItem add; add.Text(RS_(L"Pins_AddFolder")); add.Click([weak = get_weak()](auto&&, auto&&) { if (auto self = weak.get()) { self->_PinFocusedDestination(true); } }); items.Append(add);
        MenuFlyoutItem gather; gather.Text(RS_(L"Pins_Gather")); gather.Click([weak = get_weak()](auto&&, auto&&) { if (auto self = weak.get()) { self->_GatherPins(); } }); items.Append(gather); items.Append(MenuFlyoutSeparator{});
        if (!error.empty()) { MenuFlyoutItem invalid; invalid.Text(error); invalid.IsEnabled(false); items.Append(invalid); co_return; }
        for (const auto& destination : state.catalog.pins)
        {
            const auto found = state.leases.find(destination.id); const bool open = found != state.leases.end(); const bool here = open && found->second.owner == _pinOwner;
            MenuFlyoutSubItem row; row.Text(to_hstring(P::Identity(destination)) + (open ? L" [Window " + to_hstring(found->second.owner.windowId) + L"]" : L" [Closed]"));
            MenuFlyoutItem action; action.Text(RS_(here ? L"Pins_Select" : open ? L"Pins_MoveHere" : L"Pins_OpenHere"));
            action.IsEnabled(!open || found->second.transfer.empty());
            action.Click([weak = get_weak(), id = destination.id, lease = open ? std::optional{ found->second } : std::nullopt, here](auto&&, auto&&) { if (auto self = weak.get()) { if (lease && !here) { self->_RequestPinMove(*lease, false); } else { self->_OpenPinHere(id); } } }); row.Items().Append(action);
            if (here) { if (const auto t = _FindPinTab(destination.id); t && !t->Pin->dormant) { MenuFlyoutItem unpin; unpin.Text(RS_(L"Pins_Unpin")); unpin.Click([weak = get_weak(), id = destination.id](auto&&, auto&&) { if (auto self = weak.get()) { self->_Unpin(id); } }); row.Items().Append(unpin); } }
            if (!open) { MenuFlyoutItem remove; remove.Text(RS_(L"Pins_Remove")); remove.Click([weak = get_weak(), id = destination.id](auto&&, auto&&) { if (auto self = weak.get()) { self->_RemoveClosedPin(id); } }); row.Items().Append(remove); }
            items.Append(row);
        }
    }
    void TerminalPage::_RequestPinMove(const P::Lease& lease, bool background)
    {
        if (lease.owner.process != _pinOwner.process || lease.owner.started != _pinOwner.started) { _PinError(RS_(L"Pins_OtherProcess")); return; }
        const auto request = make_self<RequestReceiveContentArgs>(lease.owner.windowId, _pinOwner.windowId, 0);
        request->PinnedId(to_hstring(lease.pin)); request->PinBackground(background); RequestReceiveContent.raise(*this, *request);
    }
    safe_void_coroutine TerminalPage::_GatherPins()
    {
        const auto strong = get_strong(); const auto dispatcher = Dispatcher(); P::Snapshot state; hstring error;
        try { _EnsurePins(); } catch (...) { _PinError(PinFailure()); co_return; }
        const auto store = _pinStore; co_await resume_background(); try { state = store->Get(); } catch (...) { error = PinFailure(); }
        co_await wil::resume_foreground(dispatcher); if (_pinsClosing) { co_return; } if (!error.empty()) { _PinError(error); co_return; }
        for (const auto& pin : state.catalog.pins) { const auto it = state.leases.find(pin.id); if (it == state.leases.end()) { _OpenPinHere(pin.id, false); } else if (it->second.owner != _pinOwner && it->second.transfer.empty()) { _RequestPinMove(it->second, true); } }
    }
    bool TerminalPage::TryAttachPinContent(IVector<ActionAndArgs> actions, uint32_t index, bool restoring)
    {
        if (!actions || !actions.Size()) { return false; }
        const auto args = TerminalArgs(actions.GetAt(0));
        if (!args || args.PinnedId().empty() || (!restoring && args.PinnedTransfer().empty())) { return false; }
        try
        {
            _EnsurePins(); const auto id = P::Guid(to_string(args.PinnedId()));
            if (const auto existing = _FindPinTab(id))
            {
                if (!existing->Pin->busy && !existing->Pin->lease)
                {
                    existing->PinBusy(true); existing->Pin->transfer = to_string(args.PinnedTransfer());
                    if (!args.PinnedBackground()) { _tabView.SelectedItem(existing->TabViewItem()); }
                    _FinishPinReference(*existing, ActionAndArgs::Serialize(actions), restoring);
                }
                return true;
            }
            PinTabView view; view.id = id; view.busy = true; view.transfer = to_string(args.PinnedTransfer());
            const auto tab = _AddPinEntry(std::move(view), !args.PinnedBackground(), index);
            _FinishPinReference(tab, ActionAndArgs::Serialize(actions), restoring);
        }
        catch (...) { _PinError(PinFailure()); }
        return true;
    }
    safe_void_coroutine TerminalPage::_FinishPinReference(TerminalApp::Tab tab, hstring serialized, bool restoring)
    {
        const auto strong = get_strong(); const auto dispatcher = Dispatcher(); const auto t = _GetTabImpl(tab); const auto id = t->Pin->id; const auto ticket = t->Pin->transfer;
        const auto store = _pinStore; const auto owner = _pinOwner; std::optional<P::Lease> lease; std::optional<P::Destination> destination; hstring error;
        co_await resume_background();
        try
        {
            if (!ticket.empty()) { lease = store->AcceptTransfer(id, ticket, owner); }
            else if (restoring) { lease = store->Claim(id, owner); }
            if (!lease) { throw std::runtime_error("This pin is already open, moving, or no longer remembered. Use the Pins menu to locate it."); }
            const auto state = store->Get(); for (const auto& pin : state.catalog.pins) { if (pin.id == id) { destination = pin; } }
            if (!destination) { throw std::runtime_error("The remembered pin was removed."); }
        }
        catch (...) { error = PinFailure(); }
        co_await wil::resume_foreground(dispatcher); uint32_t index = 0;
        if (_pinsClosing || !_tabs.IndexOf(tab, index)) { if (lease && ticket.empty()) { _ReleasePin(*lease); } co_return; }
        if (!error.empty()) { t->PinBusy(false); t->Pin->placeholder->Message(error); co_return; }
        t->Pin->destination = destination; t->Pin->lease = lease;
        try { t->Pin->icon = _PinProfile(*destination).Icon().Resolved(); } CATCH_LOG()
        t->Pin->placeholder->Identity(to_hstring(P::Identity(*destination)), t->Pin->icon); t->UpdateTitle();
        const auto actions = ActionAndArgs::Deserialize(serialized); const auto first = TerminalArgs(actions.GetAt(0));
        const bool liveTransfer = !ticket.empty() && first.ContentId() != 0;
        if (liveTransfer)
        {
            try
            {
                if (!HasContents(actions, _manager)) { throw std::runtime_error("The transferred session is unavailable. A replacement session was not started."); }
                const auto selected = _tabView.SelectedItem(); const bool active = selected == tab.TabViewItem();
                auto view = *t->Pin; view.dormant = false; view.busy = true; view.placeholder = nullptr;
                first.PinnedId({}); first.PinnedTransfer({}); first.PinnedBackground(false);
                t->SetPin({}); _nextPinView = std::move(view); ++_pinActionDepth;
                auto batch = wil::scope_exit([&] { --_pinActionDepth; _nextPinView.reset(); });
                AttachContent(actions, index); _RemoveTab(tab);
                if (!active && selected) { _tabView.SelectedItem(selected); }
                _NormalizePinOrder();
            }
            catch (...) { error = PinFailure(); if (t->Pin) { t->PinBusy(false); t->Pin->placeholder->Message(error); } _PinError(error); co_return; }
        }
        else { t->PinBusy(false); t->Pin->placeholder->Message(RS_(L"Pins_Dormant")); }
        if (!ticket.empty())
        {
            co_await resume_background(); try { store->CompleteTransfer(id, ticket, owner, lease->nonce); lease->transfer.clear(); } catch (...) { error = PinFailure(); }
            co_await wil::resume_foreground(dispatcher);
            if (_pinsClosing) { if (lease->transfer.empty()) { ReleaseWindow(store, owner); } co_return; }
            if (const auto current = _FindPinTab(id)) { current->Pin->lease = lease; current->PinBusy(false); }
            if (!error.empty()) { _PinError(error); }
        }
        _MaybeActivatePin();
    }
    safe_void_coroutine TerminalPage::_SendPinnedTab(com_ptr<Tab> tab, hstring window, uint32_t index, std::optional<Point> point, bool background)
    {
        const auto strong = get_strong(); const auto dispatcher = Dispatcher();
        if (!tab->Pin || tab->Pin->busy || !tab->Pin->lease) { co_return; }
        const auto lease = *tab->Pin->lease; const auto store = _pinStore; tab->PinBusy(true); std::string ticket; hstring error; uint64_t target = 0;
        try { size_t used = 0; target = std::stoull(to_string(window), &used); if (used != window.size()) { target = 0; } } catch (...) {}
        co_await resume_background(); try { ticket = store->BeginTransfer(lease.pin, lease.owner, lease.nonce, target); } catch (...) { error = PinFailure(); }
        co_await wil::resume_foreground(dispatcher);
        if (!error.empty()) { if (tab->Pin) { tab->PinBusy(false); } _PinError(error); co_return; }
        if (_pinsClosing) { _WatchPinTransfer(lease.pin, ticket, lease.owner, {}); co_return; }
        try
        {
            std::vector<ActionAndArgs> actions;
            if (tab->Pin->dormant) { NewTerminalArgs args; args.PinnedId(to_hstring(lease.pin)); actions.emplace_back(ShortcutAction::NewTab, NewTabArgs{ args }); }
            else { actions = tab->BuildStartupActions(BuildStartupKind::Content); }
            auto args = TerminalArgs(actions.at(0)); args.PinnedId(to_hstring(lease.pin)); args.PinnedTransfer(to_hstring(ticket)); args.PinnedBackground(background);
            const auto serialized = ActionAndArgs::Serialize(single_threaded_vector<ActionAndArgs>(std::vector<ActionAndArgs>{ actions }));
            _WatchPinTransfer(lease.pin, ticket, lease.owner, serialized);
            if (!tab->Pin->dormant) { _DetachTabFromWindow(tab); }
            _MoveContent(std::move(actions), window, index, point); _RemoveTab(*tab);
        }
        catch (...) { tab->PinBusy(false); _PinError(PinFailure()); _WatchPinTransfer(lease.pin, ticket, lease.owner, {}); }
    }
    safe_void_coroutine TerminalPage::_WatchPinTransfer(std::string id, std::string ticket, P::Owner source, hstring serialized)
    {
        const auto weak = get_weak(); const auto dispatcher = Dispatcher(); const auto store = _pinStore; const auto manager = _manager;
        for (;;)
        {
            co_await resume_after(std::chrono::seconds{ 31 }); co_await resume_background();
            bool cleanup = false;
            try
            {
                auto state = store->Get(); const auto found = state.leases.find(id);
                if (found == state.leases.end() || found->second.transfer != ticket) { co_return; }
                const auto owner = found->second.owner;
                if (owner != source && P::Probe(owner) == P::Life::Alive) { continue; }
                cleanup = true; // Expired fence: acceptance can no longer race recovery.
                if (owner == source && P::Probe(source) == P::Life::Alive)
                {
                    auto lease = store->RecoverTransfer(id, ticket, source);
                    co_await wil::resume_foreground(dispatcher);
                    if (const auto self = weak.get(); self && !self->_pinsClosing)
                    {
                        if (const auto existing = self->_FindPinTab(id)) { existing->Pin->lease = lease; existing->PinBusy(false); }
                        else
                        {
                            PinTabView view; view.id = id; view.lease = lease;
                            for (const auto& pin : state.catalog.pins) { if (pin.id == id) { view.destination = pin; } }
                            const auto actions = serialized.empty() ? nullptr : ActionAndArgs::Deserialize(serialized);
                            if (actions && TerminalArgs(actions.GetAt(0)).ContentId())
                            {
                                if (!HasContents(actions, manager)) { throw std::runtime_error("The failed transfer no longer has all its sessions."); }
                                view.dormant = false; auto first = TerminalArgs(actions.GetAt(0)); first.PinnedId({}); first.PinnedTransfer({});
                                self->_nextPinView = std::move(view); self->AttachContent(actions, 0); self->_nextPinView.reset(); self->_NormalizePinOrder();
                            }
                            else { self->_AddPinEntry(std::move(view), false); }
                        }
                        self->_PinError(L"The pin could not be moved. It was returned to this window.");
                        cleanup = false; // A failed completion write must not dispose a reattached live session.
                        co_await resume_background(); store->CompleteTransfer(id, ticket, source, lease.nonce); co_return;
                    }
                }
            }
            CATCH_LOG()
            if (cleanup)
            {
                co_await resume_background();
                try
                {
                    // Close detached content before releasing its exclusive fence,
                    // even when the original XAML dispatcher is already gone.
                    if (!serialized.empty()) { for (const auto& action : ActionAndArgs::Deserialize(serialized)) { if (const auto args = TerminalArgs(action); args && args.ContentId()) { if (const auto core = manager.TryLookupCore(args.ContentId())) { core.Close(); } } } }
                    store->FinishFailedTransfer(id, ticket, std::nullopt); co_return;
                }
                CATCH_LOG()
            }
        }
    }
}
