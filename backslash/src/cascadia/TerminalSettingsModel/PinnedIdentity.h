// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include "PinnedCapture.h"
#include "../inc/ConnectionIdentityTitle.h"

namespace Microsoft::Terminal::Backslash::Pins
{
    struct LaunchIdentity
    {
        winrt::guid profile;
        std::string source;
        std::string identity;
        std::wstring commandline;
    };
    template<typename Control> LaunchIdentity InitialIdentity(const Control& control)
    {
        return { control.InitialLaunchProfileId(), winrt::to_string(control.InitialLaunchProfileSource()),
            winrt::to_string(control.InitialLaunchConnectionIdentity()), std::wstring{ control.InitialLaunchCommandline() } };
    }
    template<typename Profile> ProfileTarget TargetForProfile(const Profile& profile, bool elevated)
    {
        namespace C = ::Microsoft::Terminal::Backslash::ConnectionIdentityTitle;
        ProfileTarget target;
        target.profile = winrt::to_string(::Microsoft::Console::Utils::GuidToString(profile.Guid()));
        target.source = winrt::to_string(profile.Source()); target.active = true; target.elevationCompatible = !profile.Elevate() || elevated;
        const auto identity = winrt::to_string(profile.ConnectionIdentity(profile.Commandline()));
        if (target.source == "Windows.Terminal.SSH")
        {
            target.kind = Kind::SshWindows;
            if (identity.starts_with("SSH ")) { target.host = identity.substr(4); target.supportsFixedPath = profile.SshShell() == 3; }
        }
        else if (target.source == "Windows.Terminal.Wsl")
        {
            target.kind = Kind::Wsl;
            if (identity.starts_with("WSL ")) { target.host = identity.substr(4); target.supportsFixedPath = true; }
        }
        else
        {
            target.kind = Kind::Local; target.host = winrt::to_string(C::LocalMachineName());
            target.supportsFixedPath = C::Classify(profile.Commandline()) == C::LaunchKind::LocalShell &&
                (target.source.empty() || target.source == "Windows.Terminal.PowershellCore");
        }
        return target;
    }
    inline void PreflightCapture(const ProfileTarget& target, const LaunchIdentity& launch)
    {
        namespace C = ::Microsoft::Terminal::Backslash::ConnectionIdentityTitle;
        const winrt::guid expected = ::Microsoft::Console::Utils::GuidFromString(winrt::to_hstring(Guid(target.profile)).c_str());
        bool matches = target.active && target.supportsFixedPath && target.elevationCompatible && expected == launch.profile && target.source == launch.source && !target.host.empty();
        if (matches && target.kind == Kind::Local)
        {
            const auto host = winrt::to_hstring(target.host), actual = winrt::to_hstring(launch.identity);
            matches = C::Classify(launch.commandline) == C::LaunchKind::LocalShell &&
                CompareStringOrdinal(host.c_str(), static_cast<int>(host.size()), actual.c_str(), static_cast<int>(actual.size()), TRUE) == CSTR_EQUAL;
        }
        else if (matches && target.kind == Kind::Wsl)
        {
            matches = C::Classify(launch.commandline) == C::LaunchKind::Wsl && launch.identity == "WSL " + target.host;
        }
        else if (matches)
        {
            matches = C::Classify(launch.commandline) == C::LaunchKind::Ssh && launch.identity == "SSH " + target.host;
        }
        if (!matches) { throw std::invalid_argument("The live connection does not match the current profile. Reopen that profile before pinning."); }
    }
    inline DirectoryRequest PrepareCapture(const ProfileTarget& target, const LaunchIdentity& launch,
                                           const std::string& reported, const std::string& starting,
                                           const std::string& home, const ExpandLocalDirectory& expand)
    {
        PreflightCapture(target, launch);
        return RequestDirectory(target.kind, reported, starting, home, expand);
    }
}
