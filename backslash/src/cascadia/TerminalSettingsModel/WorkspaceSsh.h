// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include "SshCommandline.h"
#include <sstream>
#include <map>
#include <set>

// Deliberately not an OpenSSH evaluator: no Include, Match, expansion, exec,
// credentials, files or network operations. Unsupported input blocks import.
namespace Microsoft::Terminal::Backslash::WorkspaceSsh
{
    inline constexpr std::wstring_view Source = L"Backslash.Terminal.SSH";
    struct Record
    {
        std::string alias, host, user;
        uint32_t port = 22;
        bool hostAlias = false; // Reference only: this computer supplies config and authentication.
        std::string profile; // Optional legacy identity retained when Save adopts an edited discovery.
        bool operator==(const Record&) const = default;
    };
    inline bool Atom(std::string_view value, bool host = false)
    {
        if (value.empty() || value.size() > 253 || value.front() == '-') { return false; }
        return std::all_of(value.begin(), value.end(), [host](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || (host && c == ':');
        });
    }
    inline std::string Lower(std::string value)
    {
        for (auto& c : value) { if (c >= 'A' && c <= 'Z') { c += 'a' - 'A'; } } return value;
    }
    inline Json::Value Value(const Record& r)
    {
        Json::Value v{ Json::objectValue };
        if (r.hostAlias) { v["mode"] = "hostAlias"; v["alias"] = r.alias; if (!r.profile.empty()) { v["profile"] = r.profile; } return v; }
        v["alias"] = r.alias; v["host"] = r.host; v["user"] = r.user; v["port"] = r.port; return v;
    }
    inline Record Read(const Json::Value& v)
    {
        if (v.isObject() && v.isMember("mode"))
        {
            if ((v.size() != 2 && v.size() != 3) || (v.size() == 3 && !v.isMember("profile")) || !v["mode"].isString() || v["mode"].asString() != "hostAlias" || !v["alias"].isString() || !Atom(v["alias"].asString()))
            { throw std::invalid_argument("Invalid host SSH alias reference."); }
            Record r{v["alias"].asString(), {}, {}, 22, true};
            if (v.isMember("profile"))
            {
                if (!v["profile"].isString()) { throw std::invalid_argument("Invalid SSH profile identity."); }
                r.profile = v["profile"].asString();
                try
                {
                    const winrt::guid id{winrt::to_hstring(r.profile)};
                    if (id == winrt::guid{} || Lower(r.profile) != Lower(winrt::to_string(winrt::to_hstring(id)))) { throw std::invalid_argument("Invalid SSH profile identity."); }
                }
                catch (...) { throw std::invalid_argument("Invalid SSH profile identity."); }
            }
            return r;
        }
        if (!v.isObject() || v.size() != 4 || !v["alias"].isString() || !v["host"].isString() || !v["user"].isString() || !v["port"].isUInt()) { throw std::invalid_argument("Invalid workspace SSH record."); }
        Record r{ v["alias"].asString(), v["host"].asString(), v["user"].asString(), v["port"].asUInt() };
        if (!Atom(r.alias) || !Atom(r.host, true) || !Atom(r.user) || !r.port || r.port > 65535) { throw std::invalid_argument("Workspace SSH needs a simple alias, host, user and port (1-65535)."); }
        return r;
    }
    inline std::vector<Record> ReadRecords(const Json::Value& value)
    {
        if (!value.isArray() || value.size() > 512) { throw std::invalid_argument("Invalid workspace SSH collection."); }
        std::vector<Record> result; std::set<std::string> aliases, identities;
        for (const auto& item : value) { auto r = Read(item); if (!aliases.insert(Lower(r.alias)).second) { throw std::invalid_argument("Duplicate workspace SSH alias."); } if (!r.profile.empty() && !identities.insert(Lower(r.profile)).second) { throw std::invalid_argument("Duplicate SSH profile identity."); } result.push_back(std::move(r)); }
        return result;
    }
    struct Review
    {
        std::optional<Record> record;
        std::vector<std::string> issues;
        std::string Description() const
        {
            std::string text;
            if (record) { text = record->alias + " -> " + record->user + "@" + record->host + ":" + std::to_string(record->port) + "\n\nThis copies only the connection address into the workspace. Requires an available Windows OpenSSH client. Authentication is prompted for each new connection; no keys, credentials or host trust are copied. Server fingerprints use the workspace known-hosts file. Save Settings to keep the import."; }
            for (const auto& issue : issues) { text += "\n" + issue; }
            return text;
        }
    };
    inline Review Preview(std::string alias, std::string_view userConfig, std::string_view systemConfig, std::string defaultUser)
    {
        Review result;
        if (!Atom(alias)) { result.issues.push_back("This alias needs manual configuration; only simple SSH aliases can be imported."); return result; }
        std::map<std::string, std::string> options; bool seen = false;
        for (const auto config : { userConfig, systemConfig })
        {
            if (config.size() > 1024 * 1024) { result.issues.push_back("SSH config exceeds the 1 MiB import limit."); continue; }
            std::istringstream input{ std::string{ config } }; std::string line; bool applies = true;
            while (std::getline(input, line))
            {
                if (line.size() > 32760 || line.find('\0') != line.npos) { result.issues.push_back("Malformed SSH configuration."); break; }
                for (size_t i = 0; i < line.size(); ++i)
                {
                    if (line[i] == '#' && (i == 0 || line[i - 1] == ' ' || line[i - 1] == '\t')) { line.resize(i); break; }
                }
                const auto start = line.find_first_not_of(" \t\r"); if (start == line.npos) { continue; }
                line.erase(0, start); const auto end = line.find_first_of(" \t=");
                if (end == line.npos) { result.issues.push_back("Malformed SSH directive."); continue; }
                const auto key = Lower(line.substr(0, end)); auto value = line.substr(end);
                size_t first = 0;
                while (first < value.size() && (value[first] == ' ' || value[first] == '\t')) { ++first; }
                if (first < value.size() && value[first] == '=') { ++first; }
                while (first < value.size() && (value[first] == ' ' || value[first] == '\t')) { ++first; }
                if (first == value.size()) { result.issues.push_back("Missing SSH directive value."); continue; }
                if (value[first] == '=') { result.issues.push_back("Malformed SSH separator: only one optional = is supported."); continue; }
                value.erase(0, first); value.erase(value.find_last_not_of(" \t\r") + 1);
                if (key == "include" || key == "match") { result.issues.push_back("Import blocked by " + key + ": its dependencies/conditions are not evaluated or executed. Configure this connection explicitly instead."); continue; }
                if (key == "host")
                {
                    applies = false; std::istringstream names{ value }; std::string name;
                    while (names >> name)
                    {
                        if (name == "*") { applies = true; }
                        else if (!Atom(name)) { result.issues.push_back("Host patterns other than * require manual review."); }
                        else if (Lower(name) == Lower(alias)) { applies = true; seen = true; }
                    }
                    continue;
                }
                if (!applies) { continue; }
                if (key != "hostname" && key != "user" && key != "port")
                {
                    // Never silently drop a key/proxy/agent/trust/path/command option.
                    result.issues.push_back("Import blocked by option " + key + ". Its authentication, path or other behavior is not copied. Keep this connection on this computer or configure it explicitly."); continue;
                }
                options.try_emplace(key, value); // OpenSSH: user first; first value wins.
            }
        }
        if (!seen) { result.issues.push_back("The alias was removed or no longer has a simple Host entry. Refresh this computer's connections."); }
        if (!result.issues.empty()) { return result; }
        Record r{ alias, options.contains("hostname") ? options.at("hostname") : alias, options.contains("user") ? options.at("user") : std::move(defaultUser), 22 };
        if (options.contains("port"))
        {
            const auto& p = options.at("port");
            if (p.empty() || p.size() > 5 || !std::all_of(p.begin(), p.end(), [](char c) { return c >= '0' && c <= '9'; })) { result.issues.push_back("Invalid SSH port."); return result; }
            r.port = static_cast<uint32_t>(std::stoul(p));
        }
        try { result.record = Read(Value(r)); } catch (const std::exception& e) { result.issues.push_back(e.what()); }
        return result;
    }
    inline std::wstring BaseCommand(const Record& record, std::wstring_view exe)
    {
        namespace S = winrt::Microsoft::Terminal::Settings::Model::implementation::SshCommandline;
        if (record.hostAlias) { return S::Quote(exe) + L" " + S::Quote(winrt::to_hstring(record.alias)); }
        auto result = S::Quote(exe) + L" -F none";
        for (const auto& option : { std::wstring{ L"HostName=" } + winrt::to_hstring(record.host).c_str(),
                                   std::wstring{ L"User=" } + winrt::to_hstring(record.user).c_str(),
                                   std::wstring{ L"Port=" } + std::to_wstring(record.port),
                                   std::wstring{ L"UserKnownHostsFile=\"%BACKSLASH_SETTINGS_DIR%/ssh_known_hosts\"" },
                                   std::wstring{ L"GlobalKnownHostsFile=none" }, std::wstring{ L"StrictHostKeyChecking=ask" },
                                   std::wstring{ L"IdentityFile=none" }, std::wstring{ L"IdentityAgent=none" },
                                   std::wstring{ L"IdentitiesOnly=yes" } }) { result += L" -o " + S::Quote(option); }
        return result + L" " + S::Quote(winrt::to_hstring(record.alias));
    }
}
