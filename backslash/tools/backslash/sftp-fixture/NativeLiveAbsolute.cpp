// Owned single-connection live fixture. Never used by normal application startup.
#include <winsock2.h>
#include <iphlpapi.h>
#define BACKSLASH_SFTP_FIXTURE_DIAGNOSTICS
#include "../../../src/cascadia/inc/SftpNativeSession.h"
#include "FixtureLog.h"
#include <fstream>
#include <iostream>
using namespace Microsoft::Terminal::Backslash::Transfers::Sftp;
static const std::filesystem::path Root=L"D:\\codex-workspace\\backslash-terminal\\artifacts\\sftp\\native-absolute-20261031-f3407b61";
static const std::wstring Server=L"D:\\codex-workspace\\backslash-terminal\\artifacts\\sftp\\native-server-inspect-20261030-d581a2c7\\layout\\OpenSSH-Win64\\sshd.exe";
static const std::string Marker="Backslash native absolute fixture f3407b61";
void Check(bool b){if(!b)throw std::runtime_error("Live fixture invariant failed");}
std::string ReadFixture(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);Check(bool(f));return {std::istreambuf_iterator<char>(f),{}};}
bool Bound(DWORD pid){DWORD size=0;GetExtendedTcpTable(nullptr,&size,FALSE,AF_INET,TCP_TABLE_OWNER_PID_LISTENER,0);std::vector<unsigned char> b(size);if(GetExtendedTcpTable(b.data(),&size,FALSE,AF_INET,TCP_TABLE_OWNER_PID_LISTENER,0)!=NO_ERROR)return false;auto* t=reinterpret_cast<MIB_TCPTABLE_OWNER_PID*>(b.data());bool found=false;for(DWORD i=0;i<t->dwNumEntries;++i){auto r=t->table[i];if(r.dwOwningPid==pid){Check(r.dwLocalAddr==htonl(INADDR_LOOPBACK)&&ntohs(static_cast<u_short>(r.dwLocalPort))==48223);found=true;}}return found;}
struct ServerOwner {
 Handle job,process,stop,errors,drainDone;std::thread timer,drain;FixtureLog log{(Root/L"evidence/server.stderr").c_str()};bool stopped=false,okay=false;
 bool Stop()noexcept {
  if(stopped)return okay;stopped=true;SetEvent(stop.value);
  const BOOL killed=job.value?TerminateJobObject(job.value,0):FALSE;
  const DWORD waited=process.value?WaitForSingleObject(process.value,5000):WAIT_FAILED;DWORD code=STILL_ACTIVE;
  const BOOL queried=process.value?GetExitCodeProcess(process.value,&code):FALSE;
  if(timer.joinable())timer.join();
  if(drain.joinable()){if(WaitForSingleObject(drainDone.value,2000)!=WAIT_OBJECT_0)CancelSynchronousIo(drain.native_handle());drain.join();}
  okay=killed&&waited==WAIT_OBJECT_0&&queried&&code!=STILL_ACTIVE&&log.Good();
  std::cout<<"SERVER_TEARDOWN terminate="<<killed<<" wait="<<waited<<" exit="<<code<<" log_ok="<<log.Good()<<" truncated="<<log.Truncated()<<" confirmed="<<okay<<"\n"<<std::flush;return okay;
 }
 ServerOwner(){try{
  job.Reset(CreateJobObjectW(nullptr,nullptr));WinCheck(job.value!=nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit{};limit.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;WinCheck(SetInformationJobObject(job.value,JobObjectExtendedLimitInformation,&limit,sizeof(limit)));stop.Reset(CreateEventW(nullptr,TRUE,FALSE,nullptr));drainDone.Reset(CreateEventW(nullptr,TRUE,FALSE,nullptr));WinCheck(stop.value&&drainDone.value);
  SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};Handle errorWrite,input,output;WinCheck(CreatePipe(&errors.value,&errorWrite.value,&sa,0));WinCheck(SetHandleInformation(errors.value,HANDLE_FLAG_INHERIT,0));
  input.Reset(CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,nullptr));output.Reset(CreateFileW(L"NUL",GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,nullptr));WinCheck(input.value!=INVALID_HANDLE_VALUE&&output.value!=INVALID_HANDLE_VALUE);
  STARTUPINFOEXW si{};si.StartupInfo.cb=sizeof(si);si.StartupInfo.dwFlags=STARTF_USESTDHANDLES;si.StartupInfo.hStdInput=input.value;si.StartupInfo.hStdOutput=output.value;si.StartupInfo.hStdError=errorWrite.value;
  SIZE_T size=0;InitializeProcThreadAttributeList(nullptr,1,0,&size);std::vector<unsigned char> storage(size);si.lpAttributeList=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());WinCheck(InitializeProcThreadAttributeList(si.lpAttributeList,1,0,&size));struct Cleanup{LPPROC_THREAD_ATTRIBUTE_LIST p;~Cleanup(){DeleteProcThreadAttributeList(p);}}cleanup{si.lpAttributeList};HANDLE inherited[]{input.value,output.value,errorWrite.value};WinCheck(UpdateProcThreadAttribute(si.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,inherited,sizeof(inherited),nullptr,nullptr));
  std::wstring cmd=QuoteArgument(Server)+L" -d -e -f "+QuoteArgument((Root/L"config/sshd_config").wstring());PROCESS_INFORMATION pi{};WinCheck(CreateProcessW(Server.c_str(),cmd.data(),nullptr,nullptr,TRUE,CREATE_SUSPENDED|CREATE_NO_WINDOW|EXTENDED_STARTUPINFO_PRESENT,nullptr,Root.c_str(),&si.StartupInfo,&pi));process.Reset(pi.hProcess);Handle initial(pi.hThread);if(!AssignProcessToJobObject(job.value,process.value)){TerminateProcess(process.value,1);throw std::runtime_error("Cannot own native server");}
  std::cout<<"SERVER_PID "<<pi.dwProcessId<<"\n"<<std::flush;
  drain=std::thread([this]{char bytes[1024];DWORD n=0;while(ReadFile(errors.value,bytes,sizeof(bytes),&n,nullptr)&&n)log.Append(std::string_view(bytes,n));SetEvent(drainDone.value);});
  timer=std::thread([this]{if(WaitForSingleObject(stop.value,115000)==WAIT_TIMEOUT)TerminateJobObject(job.value,124);});
  WinCheck(ResumeThread(initial.value)!=static_cast<DWORD>(-1));errorWrite.Reset();const auto began=GetTickCount64();while(!Bound(pi.dwProcessId)){if(WaitForSingleObject(process.value,0)!=WAIT_TIMEOUT||GetTickCount64()-began>5000)throw std::runtime_error("Server did not bind reviewed loopback endpoint");Sleep(25);}
 }catch(...){Stop();throw;}}
 ~ServerOwner(){Stop();}
};
void DownloadOne(NativeSession& session,const ConnectionPlan& plan,const char* remote,const wchar_t* local){Request r{plan,Direction::Download,(Root/L"local"/local).wstring(),remote};session.Prepare(r,{});session.Transfer(r,[](Progress){},[]{return false;});session.Publish(r,{});}
int wmain(int argc,wchar_t** argv){try{Check(argc==2&&std::wstring_view(argv[1])==L"--run-reviewed-single-connection");Handle token;WinCheck(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token.value));TOKEN_ELEVATION elevation{};DWORD n=0;WinCheck(GetTokenInformation(token.value,TokenElevation,&elevation,sizeof(elevation),&n));Check(!elevation.TokenIsElevated);Check(ReadFixture(Root/L"owner.txt")==Marker);Check(ReadFixture(Root/L"remote/owner.txt")==Marker);
 for(auto p=Root;!p.empty();p=p.parent_path()){Check(!(GetFileAttributesW(p.c_str())&FILE_ATTRIBUTE_REPARSE_POINT));if(p==p.parent_path())break;}
 const auto expected=ReadFixture(Root/L"local/source.bin");Check(expected.size()==65536);for(size_t i=0;i<expected.size();++i)Check(static_cast<unsigned char>(expected[i])==static_cast<unsigned char>(i%256));
 ServerOwner server;std::cout<<"LISTENER_BOUND_127.0.0.1_48223\n"<<std::flush;
 ConnectionPlan plan{L"C:\\Windows\\System32\\OpenSSH\\ssh.exe",{L"-v",L"-F",L"none",L"-oIdentityAgent=none",L"-oIdentitiesOnly=yes",L"-oGlobalKnownHostsFile=none",L"-oUserKnownHostsFile="+(Root/L"config/known_hosts").wstring(),L"-i",(Root/L"keys/client_ed25519").wstring(),L"-p",L"48223",L"nemo@127.0.0.1"},true};
 FixtureLog clientLog((Root/L"evidence/client.stderr").c_str());
 {std::cout<<"STAGE_SSH_START\n"<<std::flush;NativeSession session(plan,[&clientLog](std::string_view bytes){clientLog.Append(bytes);});std::cout<<"STAGE_BROWSE_BEGIN\n"<<std::flush;const auto entries=session.Browse(".");std::cout<<"STAGE_BROWSE_COMPLETE\n"<<std::flush;bool marker=false;for(const auto& e:entries)if(e.name=="owner.txt")marker=true;Check(marker);
 std::cout<<"STAGE_MARKER_DOWNLOAD\n"<<std::flush;DownloadOne(session,plan,"owner.txt",L"marker-check.txt");Check(ReadFixture(Root/L"local/marker-check.txt")==Marker);
 Check(ReadFixture(Root/L"owner.txt")==Marker);Request upload{plan,Direction::Upload,(Root/L"local/source.bin").wstring(),"uploaded.bin"};std::cout<<"STAGE_UPLOAD\n"<<std::flush;session.Prepare(upload,{});session.Transfer(upload,[](Progress){},[]{return false;});session.Publish(upload,{});
 std::cout<<"STAGE_DOWNLOAD\n"<<std::flush;DownloadOne(session,plan,"uploaded.bin",L"download.bin");Check(ReadFixture(Root/L"local/download.bin")==expected);Check(ReadFixture(Root/L"remote/uploaded.bin")==expected);
 }Check(clientLog.Good());std::cout<<"CLIENT_LOG truncated="<<clientLog.Truncated()<<"\n"<<std::flush;Check(server.Stop());
 std::cout<<"PASS: one connection, production NativeSession browse, marker verification, upload/download and binary integrity. No retry/trust-rejection/GUI acceptance.\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}





