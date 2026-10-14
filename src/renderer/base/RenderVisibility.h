// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <atomic>
#include <wil/resource.h>
#include <wil/result.h>

namespace Microsoft::Console::Render
{
    // Rendering only: the connection, parser and terminal invalidations remain active.
    class RenderVisibility
    {
    public:
        RenderVisibility()
        {
            _resume.create(wil::EventOptions::ManualReset);
            _resume.SetEvent();
        }

        bool SetVisible(const bool visible) noexcept
        {
            const auto guard = _lock.lock_exclusive();
            if (_visible.load(std::memory_order_relaxed) == visible)
            {
                return false;
            }
            if (visible)
            {
                // Publish the resume invalidation before a worker can observe visibility.
                _redrawOnResume.store(true, std::memory_order_relaxed);
                _visible.store(true, std::memory_order_release);
                _resume.SetEvent();
            }
            else
            {
                _resume.ResetEvent();
                _visible.store(false, std::memory_order_release);
            }
            return true;
        }

        bool IsVisible() const noexcept { return _visible.load(std::memory_order_acquire); }
        bool IsSuspended() const noexcept { return _suspended.load(std::memory_order_acquire); }
        bool TakeResumeRedraw() noexcept { return _redrawOnResume.exchange(false, std::memory_order_acquire); }

        template<typename F>
        bool Wait(const HANDLE shutdown, F&& onSuspended) noexcept
        {
            while (!IsVisible())
            {
                _suspended.store(true, std::memory_order_release);
                onSuspended();
                const HANDLE handles[]{ shutdown, _resume.get() };
                const auto result = WaitForMultipleObjects(ARRAYSIZE(handles), handles, FALSE, INFINITE);
                FAIL_FAST_LAST_ERROR_IF(result == WAIT_FAILED);
                if (result == WAIT_OBJECT_0)
                {
                    return false;
                }
            }
            _suspended.store(false, std::memory_order_release);
            return true;
        }

        bool Wait(const HANDLE shutdown) noexcept { return Wait(shutdown, []() noexcept {}); }

    private:
        wil::srwlock _lock;
        wil::unique_event _resume;
        std::atomic<bool> _visible{ true };
        std::atomic<bool> _suspended{ false };
        std::atomic<bool> _redrawOnResume{ false };
    };
}
