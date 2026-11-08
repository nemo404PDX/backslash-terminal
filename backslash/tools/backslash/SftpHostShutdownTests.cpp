// Tests the same host gate across a DLL and real owned-process termination.
#include "../../src/cascadia/inc/HostTransferShutdown.h"
#include "../../src/cascadia/inc/SftpProcess.h"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace Microsoft::Terminal::Backslash;
void Require(bool value){if(!value)throw std::runtime_error("Host shutdown assertion failed");}
void Mark(const std::filesystem::path& root,const char* name){std::ofstream(root/name)<<"owned fixture";}
template<typename T>T Export(HMODULE module,const char* name){const auto proc=GetProcAddress(module,name);Require(proc!=nullptr);return reinterpret_cast<T>(proc);}
int Child(const std::filesystem::path& dll,const std::filesystem::path& root,const std::wstring& mode){
    HMODULE module=LoadLibraryW(dll.c_str());Require(module!=nullptr);auto start=Export<BOOL(*)(const wchar_t*,int)>(module,"StartOwned");auto entered=Export<BOOL(*)()>(module,"Entered");auto begin=Export<void(*)()>(module,"BeginOwned");auto drained=Export<BOOL(*)()>(module,"DrainedOwned");auto timeout=Export<void(*)()>(module,"TimeoutOwned");auto outcome=Export<DWORD(*)()>(module,"OutcomeOwned");
    Require(start(root.c_str(),mode==L"forced"?1:mode==L"cancel"?2:mode==L"capture"?3:0)!=FALSE);const auto started=GetTickCount64();while(!entered()){Require(GetTickCount64()-started<2000);Sleep(1);}
    if(mode==L"resident"||mode==L"multiwindow") {Require(!Shutdown::ShouldPostQuit(mode==L"multiwindow"?1:0,0,mode==L"resident"));Mark(root,"resident.txt");Sleep(150);Require(!std::filesystem::exists(root/"begin.txt"));}
    if(mode==L"unsafe-control"){Mark(root,"unsafe-before-terminate.txt");TerminateProcess(GetCurrentProcess(),0);return 0;}
    auto result=Shutdown::BeforeOwnerTermination(begin,[]{return true;},[&]{return drained()!=FALSE;},[]{Sleep(2);},[]{return GetTickCount64();},[&](auto){timeout();},[&](auto){return mode==L"forced";},mode==L"forced"||mode==L"wait"?75:2000);
    const auto flags=outcome();std::ofstream(root/"outcome.txt")<<flags;
    if(result==Shutdown::GateResult::Drained){Require(std::filesystem::exists(root/"session-cleanup.txt"));Mark(root,"drained-before-terminate.txt");}
    else{Require((flags&2)!=0&&(flags&1)==0);Mark(root,"forced-unconfirmed.txt");}
    TerminateProcess(GetCurrentProcess(),result==Shutdown::GateResult::Drained?0u:static_cast<UINT>(ERROR_TIMEOUT));return 0;
}
int wmain(int argc,wchar_t** argv){try{
    if(argc==5&&std::wstring_view(argv[1])==L"--child")return Child(argv[2],argv[3],argv[4]);Require(argc==3);const std::filesystem::path root(argv[1]),dll(argv[2]);Require(std::filesystem::exists(root));wchar_t self[32768];GetModuleFileNameW(nullptr,self,32768);
    {int prompts=0,marked=0;auto result=Shutdown::BeforeOwnerTermination([]{throw std::runtime_error("Injected bridge failure");},[]{return false;},[]{return false;},[]{},[]{return ULONGLONG{0};},[&](auto){++marked;},[&](auto){return ++prompts==2;},100);Require(result==Shutdown::GateResult::ForcedUnconfirmed&&prompts==2&&marked==2);}
    for(const auto* mode:{L"window-x",L"last-tab",L"quit",L"resident",L"multiwindow",L"capture",L"wait",L"cancel",L"forced",L"unsafe-control"}){
        const auto target=root/mode;auto command=Transfers::Sftp::QuoteArgument(self)+L" --child "+Transfers::Sftp::QuoteArgument(dll.wstring())+L" "+Transfers::Sftp::QuoteArgument(target.wstring())+L" "+mode;STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};Require(CreateProcessW(self,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi)!=0);Transfers::Sftp::Handle process(pi.hProcess),thread(pi.hThread);
        if(WaitForSingleObject(process.value,6000)!=WAIT_OBJECT_0){TerminateProcess(process.value,9);throw std::runtime_error("Owned child deadline exceeded");}DWORD code=1;GetExitCodeProcess(process.value,&code);Require(code==(std::wstring_view(mode)==L"forced"?static_cast<DWORD>(ERROR_TIMEOUT):0u));
        const bool forced=std::wstring_view(mode)==L"forced",unsafe=std::wstring_view(mode)==L"unsafe-control",canceled=std::wstring_view(mode)==L"cancel";
        Require(std::filesystem::exists(target/"published.txt")==(!forced&&!unsafe&&!canceled));Require(std::filesystem::exists(target/"drained-before-terminate.txt")==(!forced&&!unsafe));if(forced)Require(std::filesystem::exists(target/"forced-unconfirmed.txt"));if(std::wstring_view(mode)==L"capture")Require(std::filesystem::exists(target/"authority-cleanup.txt"));if(std::wstring_view(mode)==L"wait")Require(std::filesystem::exists(target/"timeout.txt"));std::wcout<<L"PASS owned process "<<mode<<L"\n";
    }
    Require(!Shutdown::ShouldPostQuit(0,1,false)&&Shutdown::ShouldPostQuit(0,0,false));std::cout<<"PASS shared host gate + owning fixture DLL + actual TerminateProcess ordering; delayed publication/capture cleanup, cancel, explicit uncertain force, wait, resident/multiwindow policy. No network.\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<std::endl;return 1;}}
