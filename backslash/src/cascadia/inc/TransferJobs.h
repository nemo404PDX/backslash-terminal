// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <Windows.h>
#include "TransferBackends.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Microsoft::Terminal::Backslash::Transfers
{
    enum class Status { Queued, Connecting, Running, Verifying, Canceling, Completed, Failed, Canceled };
    enum class Filter { All, Active, Completed, Failed, Canceled };
    struct Job
    {
        std::wstring id;
        std::wstring name;
        std::wstring source;
        std::wstring destination;
        Status status = Status::Queued;
        std::wstring detail;
        JobOwnership ownership;
    };
    constexpr bool IsActive(Status status) noexcept
    {
        return status == Status::Queued || status == Status::Connecting || status == Status::Running ||
               status == Status::Verifying || status == Status::Canceling;
    }
    inline bool Contains(std::wstring_view value, std::wstring_view query) noexcept
    {
        if (query.empty()) { return true; }
        if (query.size() > value.size() || query.size() > static_cast<size_t>(std::numeric_limits<int>::max())) { return false; }
        // Ordinal Windows Unicode comparison, independent of display locale.
        for (size_t i = 0; i <= value.size() - query.size(); ++i)
        {
            if (CompareStringOrdinal(value.data() + i, static_cast<int>(query.size()), query.data(), static_cast<int>(query.size()), TRUE) == CSTR_EQUAL) { return true; }
        }
        return false;
    }
    inline bool Matches(const Job& job, Filter filter, std::wstring_view query) noexcept
    {
        const bool status = filter == Filter::All || (filter == Filter::Active && IsActive(job.status)) ||
                            (filter == Filter::Completed && job.status == Status::Completed) ||
                            (filter == Filter::Failed && job.status == Status::Failed) ||
                            (filter == Filter::Canceled && job.status == Status::Canceled);
        return status && (Contains(job.name, query) || Contains(job.source, query) || Contains(job.destination, query) || Contains(job.detail, query));
    }
    // Process-owned records; no view, credentials, transport or persistence. A future
    // worker publishes state here. Sharing a library alone does not share live jobs.
    class Store
    {
    public:
        static Store& Shared() { static Store store; return store; }
        std::vector<Job> Snapshot() const { std::scoped_lock lock{_mutex}; return _jobs; }
        bool Upsert(Job job)
        {
            if (job.id.empty()) { return false; }
            std::vector<std::function<void()>> notify;
            {
                std::scoped_lock lock{_mutex};
                const auto it = std::find_if(_jobs.begin(), _jobs.end(), [&](const auto& item) { return item.id == job.id; });
                if (it == _jobs.end()) { _jobs.emplace_back(std::move(job)); } else { *it = std::move(job); }
                for (const auto& [id, callback] : _observers) { (void)id; notify.push_back(callback); }
            }
            for (const auto& callback : notify) { callback(); }
            return true;
        }
        uint64_t Subscribe(std::function<void()> callback)
        {
            std::scoped_lock lock{_mutex};
            const auto token = ++_nextObserver;
            _observers.emplace(token, std::move(callback)); return token;
        }
        void Unsubscribe(uint64_t token) { std::scoped_lock lock{_mutex}; _observers.erase(token); }
    private:
        mutable std::mutex _mutex;
        std::vector<Job> _jobs;
        uint64_t _nextObserver = 0;
        std::unordered_map<uint64_t, std::function<void()>> _observers;
    };
}
