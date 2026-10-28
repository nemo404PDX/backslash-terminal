// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include "../inc/SettingsViewLifecycle.h"
#include <functional>
#include <mutex>
#include <memory>

class Pane;

namespace winrt::TerminalApp::implementation
{
    // Process-local identity Settings=0. Pane/tab/window IDs remain untouched.
    // UI callbacks enqueue to their owner dispatcher; never invoke them under lock.
    class SettingsViewHost
    {
    public:
        static SettingsViewHost& Instance(::Microsoft::Terminal::Backslash::BuiltinViewId id = ::Microsoft::Terminal::Backslash::BuiltinViewId::Settings)
        {
            // Independent ownership state for each explicitly assigned built-in.
            static SettingsViewHost settings;
            static SettingsViewHost transfers;
            switch (id)
            {
            case ::Microsoft::Terminal::Backslash::BuiltinViewId::Settings: return settings;
            case ::Microsoft::Terminal::Backslash::BuiltinViewId::Transfers: return transfers;
            default: throw std::invalid_argument("Unknown built-in view identity");
            }
        }
        void PreferenceWriteGuard(std::function<bool()> guard) { std::scoped_lock lock{_mutex}; _preferenceWriteGuard = std::move(guard); }
        bool CanWritePreference()
        {
            std::function<bool()> guard;
            { std::scoped_lock lock{_mutex}; if (_state.Pending()) { return false; } if (!_state.Owner()) { return true; } guard = _preferenceWriteGuard; }
            return !guard || guard();
        }
        uint64_t Request()
        {
            std::function<void()> focus;
            uint64_t lease = 0;
            {
                std::scoped_lock lock{_mutex};
                const auto request = _state.Open();
                if (request.action == Lifecycle::Action::Create) { lease = request.lease; _creationClaimed = false; }
                else if (request.action == Lifecycle::Action::Focus) { focus = _focus; }
            }
            if (focus) { focus(); }
            return lease;
        }
        bool ClaimCreation(uint64_t lease)
        {
            std::scoped_lock lock{_mutex};
            if (!lease || lease != _state.Pending() || _creationClaimed || _transferPane) { return false; }
            _creationClaimed = true; return true;
        }
        bool FailUnclaimedCreation(uint64_t lease)
        {
            std::scoped_lock lock{_mutex};
            if (!lease || _transferPane || _creationClaimed || lease != _state.Pending() || _state.Owner()) { return false; }
            return _state.Failed(lease);
        }
        uint64_t Pending() { std::scoped_lock lock{_mutex}; return _state.Pending(); }
        bool Ready(uint64_t lease, std::function<void()> focus)
        {
            std::scoped_lock lock{_mutex};
            if (!_state.Ready(lease)) { return false; }
            _focus = std::move(focus); return true;
        }
        uint64_t BeginTransfer(uint64_t source, std::shared_ptr<Pane> pane, std::function<void()> commit, std::function<void()> rollback)
        {
            std::scoped_lock lock{_mutex};
            const auto target = _state.BeginTransfer(source);
            if (!target) { return 0; }
            _transferPane = std::move(pane); _transferThread = GetCurrentThreadId();
            _transferDispatcher = winrt::Windows::System::DispatcherQueue::GetForCurrentThread();
            _commit = std::move(commit); _rollback = std::move(rollback);
            return target;
        }
        std::shared_ptr<Pane> TakeTransfer(uint64_t lease)
        {
            winrt::Windows::System::DispatcherQueue sourceDispatcher{nullptr};
            {
                std::scoped_lock lock{_mutex};
                if (!_transferPane || lease != _state.Pending() || _transferTaken) { return nullptr; }
                if (_transferThread == GetCurrentThreadId()) { _transferTaken = true; return _transferPane; }
                sourceDispatcher = _transferDispatcher;
            }
            // Never marshal XAML across threads. Restore on the owning thread instead.
            if (sourceDispatcher) { sourceDispatcher.TryEnqueue([this, lease] { AbortTransfer(lease); }); }
            return nullptr;
        }
        bool IsTransferPane(const std::shared_ptr<Pane>& pane)
        {
            std::scoped_lock lock{_mutex}; return _transferPane && _transferPane == pane;
        }
        uint64_t TransferLease() { std::scoped_lock lock{_mutex}; return _transferPane ? _state.Pending() : 0; }
        void CommitTransfer(uint64_t lease, std::function<void()> focus)
        {
            std::function<void()> commit;
            {
                std::scoped_lock lock{_mutex};
                if (!_transferPane || !_transferTaken || _transferThread != GetCurrentThreadId() || !_state.Ready(lease)) { throw winrt::hresult_illegal_method_call(); }
                _focus = std::move(focus); commit = std::move(_commit);
                _transferPane.reset(); _rollback = {}; _transferTaken = false;
            }
            if (commit) { try { commit(); } catch (...) { LOG_CAUGHT_EXCEPTION(); } }
        }
        void AbortTransfer(uint64_t lease)
        {
            std::function<void()> rollback;
            {
                std::scoped_lock lock{_mutex};
                if (!_transferPane || _transferThread != GetCurrentThreadId() || !_state.Failed(lease)) { return; }
                rollback = std::move(_rollback); _transferPane.reset(); _commit = {}; _transferTaken = false;
            }
            if (rollback) { rollback(); }
        }
        void AbortUnclaimedTransfer(uint64_t lease)
        {
            {
                std::scoped_lock lock{_mutex};
                if (!_transferPane || _transferTaken || lease != _state.Pending()) { return; }
            }
            AbortTransfer(lease);
        }
        uint64_t Owner() { std::scoped_lock lock{_mutex}; return _state.Owner(); }
        void Release(uint64_t lease)
        {
            std::scoped_lock lock{_mutex};
            if (_state.Failed(lease)) { if (!_state.Owner()) { _focus = {}; } }
            else if (_state.Close(lease)) { _focus = {}; }
        }
    private:
        using Lifecycle = ::Microsoft::Terminal::Backslash::SettingsViewLifecycle;
        std::mutex _mutex;
        Lifecycle _state;
        bool _creationClaimed = false;
        std::function<void()> _focus;
        std::function<bool()> _preferenceWriteGuard;
        std::shared_ptr<Pane> _transferPane;
        DWORD _transferThread = 0;
        winrt::Windows::System::DispatcherQueue _transferDispatcher{nullptr};
        bool _transferTaken = false;
        std::function<void()> _commit;
        std::function<void()> _rollback;
    };
}
