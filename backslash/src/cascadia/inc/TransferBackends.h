// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <array>
#include <cstdint>
#include <string>
namespace Microsoft::Terminal::Backslash::Transfers
{
    enum class Backend { Sftp, Sabnzbd, Aria2, Rclone };
    enum class Availability { Disabled, Unconfigured, Unavailable, Ready };
    enum class ConfigurationMode { None, OwnedSshProfile, ManagedInstallation, ExistingInstallation, RemoteServer };
    enum class Capability : uint32_t { None = 0, Download = 1, Upload = 2, Torrent = 4, Nzb = 8, Cloud = 16 };
    struct BackendDescriptor { Backend id; Capability planned; };
    inline constexpr std::array Catalog{
        BackendDescriptor{Backend::Sftp, static_cast<Capability>(3)},
        BackendDescriptor{Backend::Sabnzbd, static_cast<Capability>(9)},
        BackendDescriptor{Backend::Aria2, static_cast<Capability>(5)},
        BackendDescriptor{Backend::Rclone, static_cast<Capability>(19)} };
    // Runtime evidence is supplied by the future adapter, never inferred from preferences.
    struct RuntimeAvailability { bool implemented = false; bool verified = false; Capability capabilities = Capability::None; };
    constexpr Availability Evaluate(bool enabled, bool configured, RuntimeAvailability runtime = {}) noexcept
    {
        if (!enabled) { return Availability::Disabled; }
        if (!configured) { return Availability::Unconfigured; }
        return runtime.implemented && runtime.verified ? Availability::Ready : Availability::Unavailable;
    }
    constexpr bool CanSubmit(Availability status, Capability actual, Capability requested) noexcept
    {
        return status == Availability::Ready && requested != Capability::None &&
            (static_cast<uint32_t>(actual) & static_cast<uint32_t>(requested)) == static_cast<uint32_t>(requested);
    }
    // New submissions consult enabled state. Existing jobs retain their worker and
    // configuration snapshot; disabling or closing any UI never cancels them.
    struct JobOwnership
    {
        std::wstring backendInstance;
        std::wstring owningMachine;
        std::wstring destination; // On owningMachine unless explicitly identified otherwise.
    };
}
