// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "../../src/cascadia/inc/SftpProcess.h"
#include <iostream>
#include <future>
using namespace Microsoft::Terminal::Backslash::Transfers::Sftp;
void Require(bool value){if(!value)throw std::runtime_error("Transport assertion failed");}
template<typename F> void Reject(F f){bool rejected=false;try{f();}catch(const std::runtime_error&){rejected=true;}Require(rejected);}
int wmain(int argc,wchar_t** argv)
{
    std::wstring mode;
    for(int i=1;i<argc;++i) if(std::wstring_view(argv[i]).starts_with(L"--fake-")) mode=argv[i];
    if(!mode.empty()) {
        if(mode!=L"--fake-hold") {
            std::vector<std::wstring> expected{L"-oBatchMode=yes",L"-oStrictHostKeyChecking=yes",L"-oControlMaster=no",L"-oControlPath=none",L"-oControlPersist=no"};
            for(const auto& flag:expected) {bool found=false;const auto key=flag.substr(0,flag.find(L'=')+1);for(int i=1;i<argc;++i){const std::wstring arg=argv[i];if(arg.starts_with(key)){found=arg==flag;break;}}if(!found)return 90;}
        }
        if(mode==L"--fake-exit") return 7;
        DWORD written=0; const DWORD pid=GetCurrentProcessId();
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),&pid,sizeof(pid),&written,nullptr);
        if(mode==L"--fake-echo") {
            char diagnostics[4096]{};for(int i=0;i<128;++i)WriteFile(GetStdHandle(STD_ERROR_HANDLE),diagnostics,sizeof(diagnostics),&written,nullptr);
            uint8_t buffer[8];DWORD count=0;if(ReadFile(GetStdHandle(STD_INPUT_HANDLE),buffer,sizeof(buffer),&count,nullptr))WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),buffer,count,&written,nullptr);
            return 0;
        }
        if(mode==L"--fake-partial") { const uint8_t partial[]{0,0};WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),partial,sizeof(partial),&written,nullptr); }
        if(mode==L"--fake-cleanup") {
            Wire handle;handle.bytes.push_back(102);handle.U32(1);handle.String("handle");
            Wire malformed;malformed.bytes.push_back(104);malformed.U32(2);malformed.U32(1);
            for(auto* response:{&handle,&malformed}) {Wire header;header.U32(static_cast<uint32_t>(response->bytes.size()));WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),header.bytes.data(),static_cast<DWORD>(header.bytes.size()),&written,nullptr);WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),response->bytes.data(),static_cast<DWORD>(response->bytes.size()),&written,nullptr);}
        }
        if(mode==L"--fake-tree") {
            wchar_t path[32768];Require(GetModuleFileNameW(nullptr,path,32768)!=0);
            auto command=QuoteArgument(path)+L" --fake-hold";
            STARTUPINFOW si{};si.cb=sizeof(si);si.dwFlags=STARTF_USESTDHANDLES;si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);si.hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);si.hStdError=GetStdHandle(STD_ERROR_HANDLE);
            PROCESS_INFORMATION pi{};Require(CreateProcessW(path,command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi)!=0);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);
        }
        Sleep(INFINITE);return 0;
    }
    wchar_t path[32768];Require(GetModuleFileNameW(nullptr,path,32768)!=0);
    const auto started=GetTickCount64();
    { ProcessTransport p(path,{L"-oControlMaster=auto",L"-oControlPath=shared",L"-oStrictHostKeyChecking=no",L"--fake-echo"},2000);DWORD pid=0;p.Read(std::span<uint8_t>(reinterpret_cast<uint8_t*>(&pid),sizeof(pid)));Require(pid!=0);Bytes b{0,1,2,3,4,5,6,255};p.Write(b);Bytes result(8);p.Read(result);Require(result==b); }
    { ProcessTransport p(path,{L"--fake-stall"},150);DWORD pid=0;p.Read(std::span<uint8_t>(reinterpret_cast<uint8_t*>(&pid),sizeof(pid)));Bytes b(1);Reject([&]{p.Read(b);});Require(p.TimedOut()); }
    { ProcessTransport p(path,{L"--fake-stall"},150);DWORD pid=0;p.Read(std::span<uint8_t>(reinterpret_cast<uint8_t*>(&pid),sizeof(pid)));Bytes b(1024*1024);Reject([&]{p.Write(b);});Require(p.TimedOut()); }
    { ProcessTransport p(path,{L"--fake-stall"},5000);DWORD pid=0;p.Read(std::span<uint8_t>(reinterpret_cast<uint8_t*>(&pid),sizeof(pid)));auto read=std::async(std::launch::async,[&]{Bytes b(1);Reject([&]{p.Read(b);});});p.Cancel();Require(read.wait_for(std::chrono::seconds(2))==std::future_status::ready);read.get();Require(p.Canceled()&&!p.TimedOut()); }
    { ProcessTransport p(path,{L"--fake-partial"},150);DWORD pid=0;p.Read(std::span<uint8_t>(reinterpret_cast<uint8_t*>(&pid),sizeof(pid)));Client c([&](auto bytes){p.Read(bytes);},[&](auto bytes){p.Write(bytes);});Reject([&]{c.Initialize();});Require(p.TimedOut()); }
    { ProcessTransport p(path,{L"--fake-cleanup"},150);DWORD pid=0;p.Read(std::span<uint8_t>(reinterpret_cast<uint8_t*>(&pid),sizeof(pid)));Client c([&](auto bytes){p.Read(bytes);},[&](auto bytes){p.Write(bytes);});Reject([&]{c.List(".");});Require(p.TimedOut()); }
    Handle child,grandchild;
    { ProcessTransport p(path,{L"--fake-tree"},2000);DWORD ids[2]{};p.Read(std::span<uint8_t>(reinterpret_cast<uint8_t*>(ids),sizeof(ids)));child.Reset(OpenProcess(SYNCHRONIZE,FALSE,ids[0]));grandchild.Reset(OpenProcess(SYNCHRONIZE,FALSE,ids[1]));Require(child.value&&grandchild.value); }
    Require(WaitForSingleObject(child.value,2000)==WAIT_OBJECT_0);Require(WaitForSingleObject(grandchild.value,2000)==WAIT_OBJECT_0);
    { ProcessTransport p(path,{L"--fake-exit"},1000);Bytes b(1);Reject([&]{p.Read(b);}); }
    Reject([&]{ProcessTransport p(LR"(Z:\missing-sftp-fixture.exe)",{},100);});
    Require(GetTickCount64()-started<15000);
    std::cout<<"PASS: owned fake child echo/binary I/O, stderr flood drain, blocked read/write deadlines, cancellation, child+descendant teardown, early exit and launch failure. No SSH/network.\n";
}
