// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"

#include "SshHostGenerator.h"
#include "../../inc/DefaultSettings.h"

#include "DynamicProfileUtils.h"
#include "SshCommandline.h"
#include "FileUtils.h"
#include <atomic>

static constexpr std::wstring_view SshHostGeneratorNamespace{ L"Windows.Terminal.SSH" };

static constexpr std::wstring_view PROFILE_TITLE_PREFIX = L"SSH - ";
static constexpr std::wstring_view PROFILE_ICON_PATH = L"\uE977"; // PC1
static constexpr std::wstring_view GENERATOR_ICON_PATH = L"\uE969"; // StorageNetworkWireless

// OpenSSH is installed under System32 when installed via Optional Features
static constexpr std::wstring_view SSH_EXE_PATH1 = L"%SystemRoot%\\System32\\OpenSSH\\ssh.exe";

// OpenSSH (x86/x64) is installed under Program Files when installed via MSI
static constexpr std::wstring_view SSH_EXE_PATH2 = L"%ProgramFiles%\\OpenSSH\\ssh.exe";

// OpenSSH (x86) is installed under Program Files x86 when installed via MSI on x64 machine
static constexpr std::wstring_view SSH_EXE_PATH3 = L"%ProgramFiles(x86)%\\OpenSSH\\ssh.exe";

static constexpr std::wstring_view SSH_SYSTEM_CONFIG_PATH = L"%ProgramData%\\ssh\\ssh_config";
static constexpr std::wstring_view SSH_USER_CONFIG_PATH = L"%UserProfile%\\.ssh\\config";

static constexpr std::wstring_view SSH_CONFIG_HOST_KEY{ L"Host" };

using namespace ::Microsoft::Terminal::Settings::Model;
using namespace winrt::Microsoft::Terminal::Settings::Model;

/*static*/ const std::wregex SshHostGenerator::_configKeyValueRegex{ LR"(^\s*(\w+)\s+([^\s].*?)\s*$)" };

winrt::hstring _getProfileName(const std::wstring_view& hostName) noexcept
{
    return til::hstring_format(FMT_COMPILE(L"{0}{1}"), PROFILE_TITLE_PREFIX, hostName);
}

winrt::hstring _getProfileCommandLine(const std::wstring_view& sshExePath, const std::wstring_view& hostName) noexcept
{
    return winrt::hstring{ implementation::SshCommandline::Quote(sshExePath) + L" " + implementation::SshCommandline::Quote(hostName) };
}

/*static*/ bool SshHostGenerator::_tryFindSshExePath(std::wstring& sshExePath) noexcept
{
    try
    {
        for (const auto& path : { SSH_EXE_PATH1, SSH_EXE_PATH2, SSH_EXE_PATH3 })
        {
            if (std::filesystem::exists(wil::ExpandEnvironmentStringsW<std::wstring>(path.data())))
            {
                sshExePath = path;
                return true;
            }
        }
    }
    CATCH_LOG();

    return false;
}

/*static*/ bool SshHostGenerator::_tryParseConfigKeyValue(const std::wstring_view& line, std::wstring& key, std::wstring& value) noexcept
{
    try
    {
        if (!line.empty() && !line.starts_with(L"#"))
        {
            std::wstring input{ line };
            std::wsmatch match;
            if (std::regex_search(input, match, SshHostGenerator::_configKeyValueRegex))
            {
                key = match[1];
                value = match[2];
                return true;
            }
        }
    }
    CATCH_LOG();

    return false;
}

/*static*/ void SshHostGenerator::_getHostNamesFromConfigFile(const std::wstring_view& configPath, std::vector<std::wstring>& hostNames) noexcept
{
    try
    {
        const std::filesystem::path resolvedConfigPath{ wil::ExpandEnvironmentStringsW<std::wstring>(configPath.data()) };
        if (std::filesystem::exists(resolvedConfigPath))
        {
            std::wifstream inputStream(resolvedConfigPath);

            std::wstring line;
            std::wstring key;
            std::wstring value;

            while (std::getline(inputStream, line))
            {
                if (_tryParseConfigKeyValue(line, key, value))
                {
                    if (til::equals_insensitive_ascii(key, SSH_CONFIG_HOST_KEY))
                    {
                        hostNames.emplace_back(value);
                    }

                }
            }
        }
    }
    CATCH_LOG();
}

std::wstring_view SshHostGenerator::GetNamespace() const noexcept
{
    return SshHostGeneratorNamespace;
}

std::wstring_view SshHostGenerator::GetDisplayName() const noexcept
{
    return RS_(L"SshHostGeneratorDisplayName");
}

std::wstring_view SshHostGenerator::GetIcon() const noexcept
{
    return GENERATOR_ICON_PATH;
}

// Method Description:
// - Generate a list of profiles for each detected OpenSSH host.
// Arguments:
// - <none>
// Return Value:
// - <A list of SSH host profiles.>
void SshHostGenerator::GenerateProfiles(std::vector<winrt::com_ptr<implementation::Profile>>& profiles) const
{
    if (!_explicitScan && !HostDiscoveryEnabled()) { return; }
    std::wstring sshExePath;
    if (_tryFindSshExePath(sshExePath))
    {
        std::vector<std::wstring> hostNames;

        _getHostNamesFromConfigFile(SSH_SYSTEM_CONFIG_PATH, hostNames);
        _getHostNamesFromConfigFile(SSH_USER_CONFIG_PATH, hostNames);

        const auto start = profiles.size();
        GenerateForHosts(sshExePath, hostNames, profiles);
        for (auto& profile : std::span{ profiles }.subspan(start))
        {
            profile->SessionOnly(true);
            profile->Name(L"This computer: " + profile->Name());
        }
    }
}

void SshHostGenerator::GenerateForHosts(std::wstring_view sshExePath, const std::vector<std::wstring>& hosts, std::vector<winrt::com_ptr<implementation::Profile>>& profiles)
{
    for (const auto& host : hosts)
    {
        if (!implementation::SshCommandline::SingleLine(host))
        {
            continue;
        }
        // Host may name several aliases. Wildcard/negated patterns aren't launch targets.
        std::wistringstream aliases{ host };
        std::wstring alias;
        while (aliases >> alias)
        {
            if (alias.starts_with(L"#"))
            {
                break;
            }
            if (!implementation::SshCommandline::ValidHost(alias))
            {
                continue;
            }
            auto profile = CreateDynamicProfile(_getProfileName(alias));
            if (std::any_of(profiles.begin(), profiles.end(), [&](const auto& p) { return p->Guid() == profile->Guid(); }))
            {
                continue;
            }
            profile->Commandline(_getProfileCommandLine(sshExePath, alias));
            profile->Icon(winrt::hstring{ PROFILE_ICON_PATH });
            profiles.emplace_back(std::move(profile));
        }
    }
}

namespace
{
    std::atomic<bool>& HostDiscovery()
    {
        static std::atomic<bool> enabled{ false };
        return enabled;
    }
}
bool SshHostGenerator::HostDiscoveryEnabled() { return HostDiscovery().load(); }
void SshHostGenerator::HostDiscoveryEnabled(bool enabled) { HostDiscovery().store(enabled); }
::Microsoft::Terminal::Backslash::WorkspaceSsh::Review SshHostGenerator::ReviewImport(const std::string& alias)
{
    const auto read = [](std::wstring_view name) {
        const auto path = wil::ExpandEnvironmentStringsW<std::wstring>(name.data());
        std::error_code error;
        if (!std::filesystem::exists(path, error))
        {
            if (error) { throw std::runtime_error("SSH configuration could not be read."); }
            return std::string{};
        }
        if (std::filesystem::file_size(path) > 1024 * 1024) { throw std::runtime_error("SSH configuration exceeds the 1 MiB import limit."); }
        std::ifstream file{ std::filesystem::path{ path }, std::ios::binary };
        if (!file) { throw std::runtime_error("SSH configuration could not be read."); }
        return std::string{ std::istreambuf_iterator<char>{ file }, {} };
    };
    const auto user = read(SSH_USER_CONFIG_PATH), system = read(SSH_SYSTEM_CONFIG_PATH);
    return ::Microsoft::Terminal::Backslash::WorkspaceSsh::Preview(alias, user, system, winrt::to_string(wil::ExpandEnvironmentStringsW<std::wstring>(L"%USERNAME%")));
}
