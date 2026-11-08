// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <Windows.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include "SftpProtocol.h"
namespace Microsoft::Terminal::Backslash::Transfers::Sftp
{
    class Handle
    {
    public:
        HANDLE value = nullptr;
        Handle() = default;
        explicit Handle(HANDLE h) : value(h) {}
        ~Handle() { Reset(); }
        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;
        void Reset(HANDLE h = nullptr) { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); value = h; }
    };
    inline void WinCheck(BOOL success) { if (!success) throw std::runtime_error("SFTP transport Windows error " + std::to_string(GetLastError())); }
    inline std::wstring QuoteArgument(std::wstring_view value)
    {
        std::wstring output=L"\""; size_t slashes=0;
        for (auto c:value) { if(c==L'\\') {++slashes; continue;} output.append(slashes*(c==L'"'?2:1),L'\\'); slashes=0; if(c==L'"') output+=L'\\'; output+=c; }
        output.append(slashes*2,L'\\'); return output+L'"';
    }
    // argv comes from the owned saved-profile parser, never a shell command.
    // Only this process and its descendants belong to the cancellation job.
    class ProcessTransport
    {
    public:
        ProcessTransport(const std::wstring& executable, const std::vector<std::wstring>& connectionArguments, uint32_t idleTimeoutMs = 30000
#ifdef BACKSLASH_SFTP_FIXTURE_DIAGNOSTICS
            , std::function<void(std::string_view)> diagnostics = {}
#endif
        )
        {
            if (!idleTimeoutMs) throw std::invalid_argument("A finite SFTP inactivity deadline is required");
            _stop.Reset(CreateEventW(nullptr,TRUE,FALSE,nullptr)); WinCheck(_stop.value!=nullptr);
            SECURITY_ATTRIBUTES security{sizeof(security),nullptr,TRUE};
            Handle childIn, childOut, childError;
            WinCheck(CreatePipe(&childIn.value,&_input.value,&security,0));
            WinCheck(CreatePipe(&_output.value,&childOut.value,&security,0));
            WinCheck(CreatePipe(&_error.value,&childError.value,&security,0));
            WinCheck(SetHandleInformation(_input.value,HANDLE_FLAG_INHERIT,0));
            WinCheck(SetHandleInformation(_output.value,HANDLE_FLAG_INHERIT,0));
            WinCheck(SetHandleInformation(_error.value,HANDLE_FLAG_INHERIT,0));
            _job.Reset(CreateJobObjectW(nullptr,nullptr)); WinCheck(_job.value!=nullptr);
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{}; limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            WinCheck(SetInformationJobObject(_job.value,JobObjectExtendedLimitInformation,&limits,sizeof(limits)));
            STARTUPINFOEXW startup{}; startup.StartupInfo.cb=sizeof(startup);
            startup.StartupInfo.dwFlags=STARTF_USESTDHANDLES;
            startup.StartupInfo.hStdInput=childIn.value; startup.StartupInfo.hStdOutput=childOut.value; startup.StartupInfo.hStdError=childError.value;
            SIZE_T size=0; InitializeProcThreadAttributeList(nullptr,1,0,&size);
            std::vector<uint8_t> storage(size); startup.lpAttributeList=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
            WinCheck(InitializeProcThreadAttributeList(startup.lpAttributeList,1,0,&size));
            struct AttributesCleanup { LPPROC_THREAD_ATTRIBUTE_LIST list; ~AttributesCleanup(){DeleteProcThreadAttributeList(list);} } cleanup{startup.lpAttributeList};
            HANDLE inherited[]{childIn.value,childOut.value,childError.value};
            WinCheck(UpdateProcThreadAttribute(startup.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,inherited,sizeof(inherited),nullptr,nullptr));
            std::wstring command=QuoteArgument(executable);
            // Mandatory policy precedes profile options (OpenSSH uses first value).
            for(const auto* option:{L"-T",L"-oBatchMode=yes",L"-oStrictHostKeyChecking=yes",L"-oUpdateHostKeys=no",L"-oClearAllForwardings=yes",L"-oForwardAgent=no",L"-oControlMaster=no",L"-oControlPath=none",L"-oControlPersist=no",L"-oRemoteCommand=none",L"-oRequestTTY=no",L"-oConnectTimeout=15",L"-s"}) command+=L" "+QuoteArgument(option);
            for(const auto& arg:connectionArguments) { if(arg.find(L'\0')!=arg.npos) throw std::runtime_error("Invalid SSH argument"); command+=L" "+QuoteArgument(arg); }
            command+=L" sftp";
            PROCESS_INFORMATION process{};
            WinCheck(CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_SUSPENDED|CREATE_NO_WINDOW|EXTENDED_STARTUPINFO_PRESENT,nullptr,nullptr,&startup.StartupInfo,&process));
            _process.Reset(process.hProcess); Handle initialThread(process.hThread);
            if(!AssignProcessToJobObject(_job.value,_process.value)) { const auto error=GetLastError(); TerminateProcess(_process.value,1); throw std::runtime_error("Cannot own SSH child: "+std::to_string(error)); }
            if(ResumeThread(initialThread.value)==static_cast<DWORD>(-1)) { Cancel(); throw std::runtime_error("Cannot start SSH child"); }
            childIn.Reset(); childOut.Reset(); childError.Reset();
            try {
                // Drain diagnostics to avoid deadlock, but never retain server text,
                // authentication material or configuration details in job logs.
                _activity=GetTickCount64();
                _watchdog=std::thread([this,idleTimeoutMs]{
                    while(WaitForSingleObject(_stop.value,25)==WAIT_TIMEOUT) {
                        if(GetTickCount64()-_activity.load() >= idleTimeoutMs) { _timedOut=true; Cancel(); return; }
                    }
                });
                #ifdef BACKSLASH_SFTP_FIXTURE_DIAGNOSTICS
                _drain=std::thread([this,diagnostics]{ char buffer[1024]; DWORD count=0; while(ReadFile(_error.value,buffer,sizeof(buffer),&count,nullptr)&&count) {if(diagnostics){try{diagnostics(std::string_view(buffer,count));}catch(...){}}} });
#else
                _drain=std::thread([this]{ char buffer[1024]; DWORD count=0; while(ReadFile(_error.value,buffer,sizeof(buffer),&count,nullptr)&&count) {} });
#endif
            } catch (...) { Cancel(); SetEvent(_stop.value); if(_watchdog.joinable()) _watchdog.join(); throw; }
        }
        ~ProcessTransport() { Cancel(); SetEvent(_stop.value); if(_watchdog.joinable()) _watchdog.join(); if(_drain.joinable()) _drain.join(); }
        ProcessTransport(const ProcessTransport&)=delete;
        void Cancel() noexcept { _canceled=true; if(_job.value) TerminateJobObject(_job.value,1); }
        bool TimedOut() const noexcept { return _timedOut; }
        bool Canceled() const noexcept { return _canceled; }
        void Read(std::span<uint8_t> bytes) { Transfer(bytes.data(),bytes.size(),false); }
        void Write(std::span<const uint8_t> bytes) { Transfer(const_cast<uint8_t*>(bytes.data()),bytes.size(),true); }
    private:
        Handle _job,_process,_input,_output,_error,_stop;
        std::atomic<bool> _canceled=false;
        std::thread _drain,_watchdog;
        std::atomic<bool> _timedOut=false;
        std::atomic<ULONGLONG> _activity=0;
        void Transfer(uint8_t* data,size_t size,bool write)
        {
            while(size) {
                if(_canceled) throw std::runtime_error(_timedOut ? "SFTP inactivity deadline exceeded" : "SFTP operation canceled");
                DWORD count=0; const auto chunk=static_cast<DWORD>(std::min<size_t>(size,32768));
                const auto okay=write?WriteFile(_input.value,data,chunk,&count,nullptr):ReadFile(_output.value,data,chunk,&count,nullptr);
                if(!okay||!count) throw std::runtime_error(_timedOut ? "SFTP inactivity deadline exceeded" : (_canceled ? "SFTP operation canceled" : "SSH connection closed; verify host trust and noninteractive authentication in the terminal"));
                _activity=GetTickCount64(); data+=count; size-=count;
            }
        }
    };
}
