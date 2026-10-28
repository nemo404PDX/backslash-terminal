// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <cstddef>

namespace Microsoft::Terminal::Backslash::StartupGroupActivation
{
    struct Context
    {
        bool initialAppLaunch = false;
        bool explicitCommand = false;
        bool connection = false;
        bool content = false;
        bool restoredLayout = false;
        bool settingsActions = false;
        bool residentReopen = false;
    };
    constexpr bool Eligible(const Context& c) noexcept
    {
        return (c.initialAppLaunch || c.residentReopen) && !c.explicitCommand && !c.connection && !c.content && !c.restoredLayout && !c.settingsActions;
    }

    // Retain exclusions after Initialize transfers payload ownership to the page.
    class InitialPayload
    {
        bool _connection = false;
        bool _content = false;
    public:
        void Capture(bool connection, bool content) noexcept
        {
            _connection = _connection || connection;
            _content = _content || content;
        }
        bool ConnectionPresent(bool live) const noexcept { return _connection || live; }
        bool ContentPresent(bool live) const noexcept { return _content || live; }
    };

    // Count every existing AppHost, including hidden/quake and Settings-only
    // windows. Reserve before construction/Initialize can pump messages, so a
    // reentrant request cannot also claim the zero-to-one transition.
    class CreationReservation
    {
        std::size_t& _pending;
        bool _residentReopen;
    public:
        CreationReservation(std::size_t& pending, std::size_t appWindows, bool initialAppLaunch) noexcept :
            _pending{pending}, _residentReopen{!initialAppLaunch && appWindows == 0 && pending == 0}
        {
            ++_pending;
        }
        ~CreationReservation() noexcept { --_pending; }
        CreationReservation(const CreationReservation&) = delete;
        CreationReservation& operator=(const CreationReservation&) = delete;
        bool ResidentReopen() const noexcept { return _residentReopen; }
    };
}
