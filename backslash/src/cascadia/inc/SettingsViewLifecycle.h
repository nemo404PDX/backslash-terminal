// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace Microsoft::Terminal::Backslash
{
    // Separate from tab indexes, per-tab pane IDs and terminal content IDs.
    // Assign explicit values to built-ins; raising this boundary never renumbers them.
    enum class BuiltinViewId : uint32_t { Settings = 0, Transfers = 1 };
    inline constexpr uint32_t InitialBuiltinSlotCount = 10;
    constexpr bool IsBuiltinSlot(uint32_t value, uint32_t slotCount = InitialBuiltinSlotCount) noexcept { return value < slotCount; }

    // Caller serializes access (the process coordinator owns the mutex/UI dispatcher).
    // Placement is deliberately not identity. A generation is an ownership lease,
    // not another built-in view ID; stale close/ready callbacks cannot steal ownership.
    class SettingsViewLifecycle
    {
    public:
        enum class Action { Create, Focus, Wait };
        struct Request { Action action; uint64_t lease; };
        Request Open()
        {
            if (_target) { return { Action::Wait, _target }; }
            if (_owner) { return { Action::Focus, _owner }; }
            _target = _Next(); return { Action::Create, _target };
        }
        bool Ready(uint64_t lease) noexcept
        {
            if (!_target || lease != _target) { return false; }
            _owner = _target; _target = 0; return true;
        }
        uint64_t BeginTransfer(uint64_t owner)
        {
            if (!owner || owner != _owner || _target) { return 0; }
            _target = _Next(); return _target;
        }
        bool Failed(uint64_t target) noexcept
        {
            if (!target || target != _target) { return false; }
            _target = 0; return true; // Existing owner and draft remain live.
        }
        bool Close(uint64_t owner) noexcept
        {
            // Source cannot disappear before acknowledged transfer completion.
            if (!owner || owner != _owner || _target) { return false; }
            _owner = 0; return true;
        }
        uint64_t Owner() const noexcept { return _owner; }
        uint64_t Pending() const noexcept { return _target; }
    private:
        uint64_t _Next()
        {
            if (_generation == std::numeric_limits<uint64_t>::max()) { throw std::overflow_error("Settings ownership generation exhausted"); }
            return ++_generation;
        }
        uint64_t _generation = 0;
        uint64_t _owner = 0;
        uint64_t _target = 0;
    };
}
