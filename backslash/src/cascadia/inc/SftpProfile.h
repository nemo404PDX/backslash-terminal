// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <Windows.h>
#include <shellapi.h>
#include <filesystem>
#include <array>
#include "SftpProcess.h"
namespace Microsoft::Terminal::Backslash::Transfers::Sftp
{
    struct ProfileSnapshot
    {
        std::wstring id, source, baseCommandline;
        bool deleted=false, transient=false;
    };
    struct ConnectionPlan { std::wstring executable; std::vector<std::wstring> arguments; bool hostAlias=false; bool operator==(const ConnectionPlan&)const=default; };
    inline bool SingleLine(std::wstring_view value) { return value.find_first_of(std::wstring_view{L"\0\r\n",3})==value.npos; }
    inline ConnectionPlan ResolveProfile(const ProfileSnapshot& profile, const std::wstring& requestedId,
                                         const std::wstring& defaultSsh, const std::wstring& settingsDirectory)
    {
        if(profile.id.empty()||profile.id!=requestedId||profile.deleted||profile.transient||
           (profile.source!=L"Backslash.Terminal.SSH"&&profile.source!=L"Windows.Terminal.SSH")) throw std::runtime_error("Choose an available saved SSH profile");
        if(profile.baseCommandline.empty()||profile.baseCommandline.size()>32768||!SingleLine(profile.baseCommandline))throw std::runtime_error("Unsupported saved SSH command");
        int count=0;auto raw=CommandLineToArgvW(profile.baseCommandline.c_str(),&count);
        if(!raw)throw std::runtime_error("Cannot parse saved SSH command");
        std::vector<std::wstring> args;try {for(int i=0;i<count;++i)args.emplace_back(raw[i]);}catch(...){LocalFree(raw);throw;}LocalFree(raw);
        if(args.size()!=2&&args.size()!=22)throw std::runtime_error("SFTP supports owned SSH aliases and structured endpoints, not custom terminal commands");
        const auto& host=args.back();
        if(host.empty()||host.front()==L'-'||host.find_first_of(L" \t%*?!\"\\")!=host.npos)throw std::runtime_error("Unsupported SSH destination");
        ConnectionPlan plan;
        plan.executable=args[0]==L"%BACKSLASH_SSH_EXE%"?defaultSsh:args[0];
        const std::filesystem::path exe{plan.executable};
        if(!exe.is_absolute()||_wcsicmp(exe.filename().c_str(),L"ssh.exe")||plan.executable.find(L'%')!=plan.executable.npos)throw std::runtime_error("SFTP needs an absolute OpenSSH ssh.exe path");
        plan.hostAlias=args.size()==2;
        if(!plan.hostAlias) {
            if(args[1]!=L"-F"||args[2]!=L"none")throw std::runtime_error("Unsupported SSH configuration arguments");
            constexpr std::array<std::wstring_view,9> keys{L"HostName=",L"User=",L"Port=",L"UserKnownHostsFile=",L"GlobalKnownHostsFile=",L"StrictHostKeyChecking=",L"IdentityFile=",L"IdentityAgent=",L"IdentitiesOnly="};
            for(size_t i=0;i<keys.size();++i) {
                auto& option=args[4+i*2];
                if(args[3+i*2]!=L"-o"||!option.starts_with(keys[i])||option.size()==keys[i].size())throw std::runtime_error("Unsupported SSH endpoint options");
                const std::wstring token=L"%BACKSLASH_SETTINGS_DIR%";
                if(i==3&&option.find(token)!=option.npos) {
                    if(!std::filesystem::path(settingsDirectory).is_absolute()||!SingleLine(settingsDirectory)||settingsDirectory.find_first_of(L"%\"")!=settingsDirectory.npos)throw std::runtime_error("Invalid portable trust directory");
                    option.replace(option.find(token),token.size(),settingsDirectory);
                }
                if(option.find(L'%')!=option.npos)throw std::runtime_error("Unsupported SSH option expansion");
            }
        }
        plan.arguments.assign(args.begin()+1,args.end());
        return plan;
    }
}
