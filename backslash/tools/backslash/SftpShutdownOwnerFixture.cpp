// Owned test DLL: no networking or production activation hooks.
#include "../../src/cascadia/inc/SftpService.h"
#include <filesystem>
#include <fstream>
using namespace Microsoft::Terminal::Backslash::Transfers::Sftp;
namespace {
struct State{std::filesystem::path root;int mode=0;std::atomic<bool> entered=false,canceled=false;};
std::shared_ptr<State> state;std::unique_ptr<Service> service;
void Mark(const std::filesystem::path& root,const char* name){std::ofstream(root/name)<<"owned fixture";}
struct CaptureTarget:Replacement{std::shared_ptr<State> s;explicit CaptureTarget(std::shared_ptr<State> value):s(std::move(value)){}~CaptureTarget(){Sleep(50);Mark(s->root,"authority-cleanup.txt");}};
struct Delayed:Session{
    std::shared_ptr<State> s;explicit Delayed(std::shared_ptr<State> value):s(std::move(value)){}
    ~Delayed(){Sleep(50);Mark(s->root,"session-cleanup.txt");}
    std::vector<Entry> Browse(std::string_view)override{return {};}
    void Prepare(const Request&,const std::shared_ptr<Replacement>& approval)override{if(s->mode==3&&!approval)throw Conflict(std::make_shared<CaptureTarget>(s));}
    void Transfer(const Request&,const ProgressCallback& progress,const CancelCheck& check)override{if(s->mode==2){s->entered=true;Mark(s->root,"transferring.txt");while(!s->canceled){check();Sleep(5);}check();}progress({1,1});}
    void Publish(const Request&,const std::shared_ptr<Replacement>&)override{s->entered=true;Mark(s->root,"publishing.txt");Sleep(s->mode==1?2000:500);Mark(s->root,"published.txt");}
    void Cancel()noexcept override{s->canceled=true;}
};
}
extern "C" __declspec(dllexport) BOOL StartOwned(const wchar_t* root,int mode){try{state=std::make_shared<State>();state->root=root;state->mode=mode;if(std::filesystem::exists(state->root))return FALSE;std::filesystem::create_directory(state->root);service=std::make_unique<Service>([](const auto&){return std::make_shared<Delayed>(state);});service->Enabled(true);Request request;request.local=L"owned-placeholder";request.remote="/fixture";auto id=service->Submit(request);if(mode==3){const auto start=GetTickCount64();while(service->Busy()){if(GetTickCount64()-start>1500)return FALSE;Sleep(1);}const auto pending=service->Jobs().back();if(!service->Replace(id,pending.decision))return FALSE;service->Subscribe([]{if(!service->Busy()&&service->Jobs().back().published)Sleep(150);});}return TRUE;}catch(...){return FALSE;}}
extern "C" __declspec(dllexport) BOOL Entered(){return state&&state->entered;}
extern "C" __declspec(dllexport) void BeginOwned(){service->BeginShutdown(false);Mark(state->root,"begin.txt");}
extern "C" __declspec(dllexport) BOOL DrainedOwned(){return service->TryDrainComplete();}
extern "C" __declspec(dllexport) void TimeoutOwned(){service->MarkShutdownTimeout();Mark(state->root,"timeout.txt");}
extern "C" __declspec(dllexport) DWORD OutcomeOwned(){const auto job=service->Jobs().back();return (job.published?1u:0u)|(job.publicationUncertain?2u:0u)|(job.phase==JobPhase::Canceled?4u:0u);}
