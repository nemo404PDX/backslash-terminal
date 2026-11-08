// Owned single-connection live fixture. Never used by normal application startup.
#include <winsock2.h>
#include <iphlpapi.h>
#include "../../../src/cascadia/inc/SftpNativeSession.h"
#include <fstream>
#include <iostream>
using namespace Microsoft::Terminal::Backslash::Transfers::Sftp;
static const std::filesystem::path Root=L"D:\\codex-workspace\\backslash-terminal\\artifacts\\sftp\\native-combined-20261030-71d4b908";
static const std::wstring Server=L"D:\\codex-workspace\\backslash-terminal\\artifacts\\sftp\\native-server-inspect-20261030-d581a2c7\\layout\\OpenSSH-Win64\\sshd.exe";
static const std::string Marker="Backslash native combined fixture 71d4b908";
void Check(bool b){if(!b)throw std::runtime_error("Live fixture invariant failed");}
std::string ReadFixture(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);Check(bool(f));return {std::istreambuf_iterator<char>(f),{}};}
bool Bound(DWORD pid){DWORD size=0;GetExtendedTcpTable(nullptr,&size,FALSE,AF_INET,TCP_TABLE_OWNER_PID_LISTENER,0);std::vector<unsigned char> b(size);if(GetExtendedTcpTable(b.data(),&size,FALSE,AF_INET,TCP_TABLE_OWNER_PID_LISTENER,0)!=NO_ERROR)return false;auto* t=reinterpret_cast<MIB_TCPTABLE_OWNER_PID*>(b.data());bool found=false;for(DWORD i=0;i<t->dwNumEntries;++i){auto r=t->table[i];if(r.dwOwningPid==pid){Check(r.dwLocalAddr==htonl(INADDR_LOOPBACK)&&ntohs(static_cast<u_short>(r.dwLocalPort))==48223);found=true;}}return found;}
struct ServerOwner {
 Handle job,process,stop;std::thread timer;
 ServerOwner(){job.Reset(CreateJobObjectW(nullptr,nullptr));WinCheck(job.value!=nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit{};limit.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;WinCheck(SetInformationJobObject(job.value,JobObjectExtendedLimitInformation,&limit,sizeof(limit)));stop.Reset(CreateEventW(nullptr,TRUE,FALSE,nullptr));WinCheck(stop.value!=nullptr);
  std::wstring cmd=QuoteArgument(Server)+L" -d -e -f "+QuoteArgument((Root/L"config/sshd_config").wstring());STARTUPINFOW si{sizeof(si)};PROCESS_INFORMATION pi{};
  WinCheck(CreateProcessW(Server.c_str(),cmd.data(),nullptr,nullptr,FALSE,CREATE_SUSPENDED|CREATE_NO_WINDOW,nullptr,Root.c_str(),&si,&pi));process.Reset(pi.hProcess);Handle thread(pi.hThread);if(!AssignProcessToJobObject(job.value,process.value)){TerminateProcess(process.value,1);throw std::runtime_error("Cannot own native server");}
  try{timer=std::thread([this]{if(WaitForSingleObject(stop.value,115000)==WAIT_TIMEOUT)TerminateJobObject(job.value,124);});}catch(...){TerminateJobObject(job.value,1);throw;}
  if(ResumeThread(thread.value)==static_cast<DWORD>(-1)){SetEvent(stop.value);timer.join();TerminateJobObject(job.value,1);throw std::runtime_error("Cannot resume server");}
  try{const auto begin=GetTickCount64();while(!Bound(pi.dwProcessId)){if(WaitForSingleObject(process.value,0)!=WAIT_TIMEOUT||GetTickCount64()-begin>5000){SetEvent(stop.value);timer.join();TerminateJobObject(job.value,1);throw std::runtime_error("Server did not bind reviewed loopback endpoint");}Sleep(25);}}catch(...){SetEvent(stop.value);if(timer.joinable())timer.join();TerminateJobObject(job.value,1);throw;}
 }
 ~ServerOwner(){TerminateJobObject(job.value,0);WaitForSingleObject(process.value,5000);SetEvent(stop.value);if(timer.joinable())timer.join();}
};
void DownloadOne(NativeSession& session,const ConnectionPlan& plan,const char* remote,const wchar_t* local){Request r{plan,Direction::Download,(Root/L"local"/local).wstring(),remote};session.Prepare(r,{});session.Transfer(r,[](Progress){},[]{return false;});session.Publish(r,{});}
int wmain(int argc,wchar_t** argv){try{Check(argc==2&&std::wstring_view(argv[1])==L"--run-reviewed-single-connection");Handle token;WinCheck(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token.value));TOKEN_ELEVATION elevation{};DWORD n=0;WinCheck(GetTokenInformation(token.value,TokenElevation,&elevation,sizeof(elevation),&n));Check(!elevation.TokenIsElevated);Check(ReadFixture(Root/L"owner.txt")==Marker);Check(ReadFixture(Root/L"remote/owner.txt")==Marker);
 for(auto p=Root;!p.empty();p=p.parent_path()){Check(!(GetFileAttributesW(p.c_str())&FILE_ATTRIBUTE_REPARSE_POINT));if(p==p.parent_path())break;}
 const auto expected=ReadFixture(Root/L"local/source.bin");Check(expected.size()==65536);for(size_t i=0;i<expected.size();++i)Check(static_cast<unsigned char>(expected[i])==static_cast<unsigned char>(i%256));
 ServerOwner server;std::cout<<"LISTENER_BOUND_127.0.0.1_48223\n"<<std::flush;
 ConnectionPlan plan{L"C:\\Windows\\System32\\OpenSSH\\ssh.exe",{L"-F",L"none",L"-oIdentityAgent=none",L"-oIdentitiesOnly=yes",L"-oGlobalKnownHostsFile=none",L"-oUserKnownHostsFile="+(Root/L"config/known_hosts").wstring(),L"-i",(Root/L"keys/client_ed25519").wstring(),L"-p",L"48223",L"nemo@127.0.0.1"},true};
 NativeSession session(plan);const auto entries=session.Browse(".");bool marker=false;for(const auto& e:entries)if(e.name=="owner.txt")marker=true;Check(marker);
 DownloadOne(session,plan,"owner.txt",L"marker-check.txt");Check(ReadFixture(Root/L"local/marker-check.txt")==Marker);
 Check(ReadFixture(Root/L"owner.txt")==Marker);Request upload{plan,Direction::Upload,(Root/L"local/source.bin").wstring(),"uploaded.bin"};session.Prepare(upload,{});session.Transfer(upload,[](Progress){},[]{return false;});session.Publish(upload,{});
 DownloadOne(session,plan,"uploaded.bin",L"download.bin");Check(ReadFixture(Root/L"local/download.bin")==expected);Check(ReadFixture(Root/L"remote/uploaded.bin")==expected);
 std::cout<<"PASS: one connection, production NativeSession browse, marker verification, upload/download and binary integrity. No retry/trust-rejection/GUI acceptance.\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}


