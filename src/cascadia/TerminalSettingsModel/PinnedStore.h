// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "PinnedDestinations.h"
#include <filesystem>
#include <map>
#include <windows.h>
#include <wil/resource.h>
#include <til/io.h>

namespace Microsoft::Terminal::Backslash::Pins
{
    struct Owner
    {
        std::string window;
        uint32_t process = 0;
        uint64_t started = 0;
        uint64_t windowId = 0;
        uint64_t hwnd = 0;
        bool operator==(const Owner&) const = default;
    };
    enum class Life { Alive, WindowGone, ProcessGone };
    struct Lease
    {
        std::string pin;
        Owner owner;
        std::string nonce;
        std::string transfer;
        uint64_t targetWindow = 0;
        uint64_t transferUntil = 0;
    };
    struct Snapshot
    {
        Catalog catalog;
        std::map<std::string, Lease> leases;
    };

    inline std::string NewId()
    {
        GUID id{}; THROW_IF_FAILED(CoCreateGuid(&id));
        return Guid(winrt::to_string(::Microsoft::Console::Utils::GuidToString(id)));
    }
    inline uint64_t FileTimeValue(FILETIME time)
    {
        return (static_cast<uint64_t>(time.dwHighDateTime) << 32) | time.dwLowDateTime;
    }
    inline uint64_t Now()
    {
        FILETIME time{}; GetSystemTimeAsFileTime(&time); return FileTimeValue(time);
    }
    inline uint64_t ProcessStarted(HANDLE process)
    {
        FILETIME born{}, exit{}, kernel{}, user{};
        THROW_IF_WIN32_BOOL_FALSE(GetProcessTimes(process, &born, &exit, &kernel, &user));
        return FileTimeValue(born);
    }
    inline std::wstring LifetimeName(const Owner& owner)
    {
        return L"Local\\Backslash.PinnedWindow." + std::wstring{ winrt::to_hstring(Guid(owner.window)) };
    }
    inline Life Probe(const Owner& owner)
    {
        wil::unique_handle process{ OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, owner.process) };
        if (!process) { return GetLastError() == ERROR_INVALID_PARAMETER ? Life::ProcessGone : Life::Alive; }
        if (WaitForSingleObject(process.get(), 0) == WAIT_OBJECT_0 || ProcessStarted(process.get()) != owner.started) { return Life::ProcessGone; }
        const auto hwnd = reinterpret_cast<HWND>(owner.hwnd);
        DWORD actualProcess = 0;
        if (!IsWindow(hwnd) || !GetWindowThreadProcessId(hwnd, &actualProcess) || actualProcess != owner.process) { return Life::WindowGone; }
        wil::unique_handle lifetime{ OpenEventW(SYNCHRONIZE, FALSE, LifetimeName(owner).c_str()) };
        if (!lifetime && GetLastError() == ERROR_FILE_NOT_FOUND) { return Life::WindowGone; }
        return Life::Alive; // Access denied is conservative: never steal a live claim.
    }

    // A per-copy file lock and one atomic envelope prevent stale settings/window
    // snapshots from overwriting the catalog or creating a second attachment.
    // Runtime leases are separate from immutable remembered descriptors.
    class Store
    {
        class Lock
        {
            wil::unique_hfile _file;
            OVERLAPPED _range{};
        public:
            explicit Lock(const std::filesystem::path& root)
            {
                std::filesystem::create_directories(root);
                _file.reset(CreateFileW((root / L"pins.lock").c_str(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
                THROW_LAST_ERROR_IF(!_file);
                const auto deadline = GetTickCount64() + 2000;
                while (!LockFileEx(_file.get(), LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &_range))
                {
                    const auto error = GetLastError();
                    if (error != ERROR_LOCK_VIOLATION) { THROW_WIN32(error); }
                    if (GetTickCount64() >= deadline) { THROW_WIN32(ERROR_TIMEOUT); }
                    Sleep(5);
                }
            }
            ~Lock() { UnlockFileEx(_file.get(), 0, 1, 0, &_range); }
        };

        std::filesystem::path _root;
        std::function<Life(const Owner&)> _probe;
        std::function<void(const std::filesystem::path&, std::string_view)> _writer;

        static Json::Value ParseJson(const std::string& content)
        {
            if (content.empty() || content.size() > 2 * Catalog::MaxBytes) { throw std::invalid_argument("Invalid or oversized pin state; original data is preserved."); }
            Json::CharReaderBuilder builder; builder["rejectDupKeys"] = true; builder["failIfExtra"] = true; builder["stackLimit"] = 64;
            std::unique_ptr<Json::CharReader> reader{ builder.newCharReader() };
            Json::Value result; std::string errors;
            if (!reader->parse(content.data(), content.data() + content.size(), &result, &errors)) { throw std::invalid_argument("Invalid pin state JSON; original data is preserved."); }
            return result;
        }
        static Owner ReadOwner(const Json::Value& value)
        {
            Catalog::Keys(value, { "window", "process", "started", "windowId", "hwnd" });
            if (!value["process"].isUInt() || value["process"].asUInt() == 0 || !value["started"].isUInt64() || !value["windowId"].isUInt64() || !value["hwnd"].isUInt64()) { throw std::invalid_argument("Invalid pin attachment owner."); }
            return { Guid(Catalog::Text(value, "window")), value["process"].asUInt(), value["started"].asUInt64(), value["windowId"].asUInt64(), value["hwnd"].asUInt64() };
        }
        static Json::Value WriteOwner(const Owner& owner)
        {
            Json::Value value{ Json::objectValue };
            value["window"] = Guid(owner.window); value["process"] = owner.process; value["started"] = Json::UInt64{ owner.started };
            value["windowId"] = Json::UInt64{ owner.windowId }; value["hwnd"] = Json::UInt64{ owner.hwnd };
            ReadOwner(value); return value;
        }
        Snapshot Read() const
        {
            const auto path = _root / L"pins.json";
            wil::unique_hfile file{ CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr) };
            if (!file)
            {
                const auto error = GetLastError();
                if (error == ERROR_FILE_NOT_FOUND) { return {}; }
                THROW_WIN32(error);
            }
            LARGE_INTEGER size{}; THROW_IF_WIN32_BOOL_FALSE(GetFileSizeEx(file.get(), &size));
            if (size.QuadPart <= 0 || size.QuadPart > 2 * Catalog::MaxBytes) { throw std::invalid_argument("Invalid or oversized pin state; original data is preserved."); }
            std::string content(static_cast<size_t>(size.QuadPart), '\0'); DWORD read = 0;
            THROW_IF_WIN32_BOOL_FALSE(ReadFile(file.get(), content.data(), static_cast<DWORD>(content.size()), &read, nullptr));
            if (read != content.size()) { throw std::runtime_error("Incomplete pin state read."); }
            const auto root = ParseJson(content); Catalog::Keys(root, { "version", "catalog", "leases" });
            if (!root["version"].isUInt() || root["version"].asUInt() != 1 || !root["leases"].isArray() || root["leases"].size() > Catalog::MaxPins) { throw std::invalid_argument("Unsupported pin state; original data is preserved."); }
            Snapshot result; result.catalog = Catalog::Read(Json::writeString(Json::StreamWriterBuilder{}, root["catalog"]));
            for (const auto& value : root["leases"])
            {
                Catalog::Keys(value, { "pin", "owner", "nonce", "transfer", "targetWindow", "transferUntil" });
                Lease lease{ Guid(Catalog::Text(value, "pin")), ReadOwner(value["owner"]), Guid(Catalog::Text(value, "nonce")) };
                if (!value["targetWindow"].isUInt64() || !value["transferUntil"].isUInt64()) { throw std::invalid_argument("Invalid pin transfer state."); }
                lease.transfer = Catalog::Text(value, "transfer");
                if (!lease.transfer.empty()) { lease.transfer = Guid(lease.transfer); }
                lease.targetWindow = value["targetWindow"].asUInt64(); lease.transferUntil = value["transferUntil"].asUInt64();
                if (std::none_of(result.catalog.pins.begin(), result.catalog.pins.end(), [&](const auto& p) { return p.id == lease.pin; }) || !result.leases.emplace(lease.pin, lease).second) { throw std::invalid_argument("Invalid duplicate/orphan pin attachment."); }
            }
            return result;
        }
        void Write(const Snapshot& state) const
        {
            Json::Value root{ Json::objectValue }; root["version"] = 1; root["catalog"] = ParseJson(state.catalog.Write()); root["leases"] = Json::Value{ Json::arrayValue };
            for (const auto& [id, lease] : state.leases)
            {
                Json::Value row{ Json::objectValue }; row["pin"] = id; row["owner"] = WriteOwner(lease.owner); row["nonce"] = Guid(lease.nonce);
                row["transfer"] = lease.transfer; row["targetWindow"] = Json::UInt64{ lease.targetWindow }; row["transferUntil"] = Json::UInt64{ lease.transferUntil };
                root["leases"].append(std::move(row));
            }
            _writer(_root / L"pins.json", Json::writeString(Json::StreamWriterBuilder{}, root));
        }
        template<typename F> auto Edit(F&& action) const
        {
            Lock lock{ _root }; auto state = Read(); bool dirty = false;
            for (auto it = state.leases.begin(); it != state.leases.end();)
            {
                const auto life = _probe(it->second.owner);
                if (life == Life::ProcessGone || (life == Life::WindowGone && it->second.transfer.empty())) { it = state.leases.erase(it); dirty = true; }
                else { ++it; }
            }
            auto result = action(state, dirty);
            if (dirty) { Write(state); }
            return result;
        }
        static Lease& Owned(Snapshot& state, const std::string& id, const Owner& owner, const std::string& nonce)
        {
            const auto found = state.leases.find(Guid(id));
            if (found == state.leases.end() || found->second.owner != owner || found->second.nonce != nonce) { throw std::runtime_error("The pin changed windows; refresh the Pins menu and retry."); }
            return found->second;
        }
    public:
        explicit Store(std::filesystem::path root, std::function<Life(const Owner&)> probe = Probe,
                       std::function<void(const std::filesystem::path&, std::string_view)> writer = [](const auto& path, auto content) { til::io::write_utf8_string_to_file_atomic(path, content); }) :
            _root{ std::move(root) }, _probe{ std::move(probe) }, _writer{ std::move(writer) } {}

        Snapshot Get() const { return Edit([](auto& state, bool&) { return state; }); }
        Lease Add(const Destination& pin, const Owner& owner) const
        {
            return Edit([&](auto& state, bool& dirty) { state.catalog.Add(pin); Lease lease{ Guid(pin.id), owner, NewId() }; state.leases.emplace(lease.pin, lease); dirty = true; return lease; });
        }
        std::optional<Lease> Claim(const std::string& id, const Owner& owner) const
        {
            return Edit([&](auto& state, bool& dirty) -> std::optional<Lease> {
                const auto key = Guid(id);
                if (std::none_of(state.catalog.pins.begin(), state.catalog.pins.end(), [&](const auto& p) { return p.id == key; })) { return {}; }
                const auto found = state.leases.find(key);
                if (found != state.leases.end()) { return found->second.owner == owner && found->second.transfer.empty() ? std::optional{ found->second } : std::nullopt; }
                Lease lease{ key, owner, NewId() }; state.leases.emplace(key, lease); dirty = true; return lease;
            });
        }
        std::vector<std::pair<Destination, Lease>> ClaimUnviewed(const Owner& owner) const
        {
            return Edit([&](auto& state, bool& dirty) {
                std::vector<std::pair<Destination, Lease>> result;
                for (const auto& pin : state.catalog.pins) { if (!state.leases.contains(pin.id)) { Lease lease{ pin.id, owner, NewId() }; state.leases.emplace(pin.id, lease); result.emplace_back(pin, lease); dirty = true; } }
                return result;
            });
        }
        bool Release(const std::string& id, const Owner& owner, const std::string& nonce) const
        {
            return Edit([&](auto& state, bool& dirty) { const auto found = state.leases.find(Guid(id)); if (found == state.leases.end() || found->second.owner != owner || found->second.nonce != nonce || !found->second.transfer.empty()) { return false; } state.leases.erase(found); dirty = true; return true; });
        }
        bool ReleaseWindow(const Owner& owner) const
        {
            return Edit([&](auto& state, bool& dirty) { for (auto it = state.leases.begin(); it != state.leases.end();) { if (it->second.owner == owner && it->second.transfer.empty()) { it = state.leases.erase(it); dirty = true; } else { ++it; } } return dirty; });
        }
        bool Remove(const std::string& id, const Owner& owner, const std::string& nonce) const
        {
            return Edit([&](auto& state, bool& dirty) {
                if (std::none_of(state.catalog.pins.begin(), state.catalog.pins.end(), [&](const auto& pin) { return pin.id == Guid(id); })) { return false; }
                if (!Owned(state, id, owner, nonce).transfer.empty()) { throw std::runtime_error("This pin is moving; wait for the transfer to finish."); }
                state.catalog.Remove(id); state.leases.erase(Guid(id)); dirty = true; return true;
            });
        }
        std::string BeginTransfer(const std::string& id, const Owner& owner, const std::string& nonce, uint64_t targetWindow) const
        {
            return Edit([&](auto& state, bool& dirty) { auto& lease = Owned(state, id, owner, nonce); if (!lease.transfer.empty()) { throw std::runtime_error("This pin is already moving."); } lease.transfer = NewId(); lease.targetWindow = targetWindow; lease.transferUntil = Now() + 30ULL * 10000000; dirty = true; return lease.transfer; });
        }
        Lease AcceptTransfer(const std::string& id, const std::string& transfer, const Owner& target) const
        {
            return Edit([&](auto& state, bool& dirty) {
                const auto it = state.leases.find(Guid(id));
                if (it == state.leases.end() || it->second.transfer != transfer || transfer.empty() || it->second.transferUntil <= Now() ||
                    (it->second.targetWindow && it->second.targetWindow != target.windowId) || it->second.owner.process != target.process || it->second.owner.started != target.started) { throw std::runtime_error("The pin transfer is unavailable or belongs to another process."); }
                Lease lease = it->second; lease.owner = target; lease.nonce = NewId(); it->second = lease; dirty = true; return lease;
            });
        }
        bool CompleteTransfer(const std::string& id, const std::string& transfer, const Owner& owner, const std::string& nonce) const
        {
            return Edit([&](auto& state, bool& dirty) { auto& lease = Owned(state, id, owner, nonce); if (transfer.empty() || lease.transfer != transfer) { return false; } lease.transfer.clear(); lease.targetWindow = 0; lease.transferUntil = 0; dirty = true; return true; });
        }
        bool RemoveUnviewed(const std::string& id) const
        {
            return Edit([&](auto& state, bool& dirty) { const auto key = Guid(id); if (state.leases.contains(key)) { throw std::runtime_error("This pin opened in another window. Refresh the Pins menu."); } dirty = state.catalog.Remove(key) || dirty; return dirty; });
        }
        Lease RecoverTransfer(const std::string& id, const std::string& ticket, const Owner& source) const
        {
            return Edit([&](auto& state, bool& dirty) { const auto it = state.leases.find(Guid(id)); if (it == state.leases.end() || ticket.empty() || it->second.transfer != ticket || it->second.owner != source || it->second.transferUntil > Now()) { throw std::runtime_error("The pin transfer cannot be recovered by this window."); } it->second.nonce = NewId(); dirty = true; return it->second; });
        }
        bool FinishFailedTransfer(const std::string& id, const std::string& transfer, const std::optional<Owner>& restore) const
        {
            return Edit([&](auto& state, bool& dirty) {
                const auto it = state.leases.find(Guid(id)); if (it == state.leases.end() || it->second.transfer != transfer || transfer.empty()) { return false; }
                if (restore) { it->second = Lease{ Guid(id), *restore, NewId() }; } else { state.leases.erase(it); }
                dirty = true; return true;
            });
        }
    };
}
