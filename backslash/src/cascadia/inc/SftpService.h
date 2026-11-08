// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include "SftpProfile.h"
#include "SftpTransfer.h"
#include <algorithm>
#include <condition_variable>
#include <optional>
namespace Microsoft::Terminal::Backslash::Transfers::Sftp
{
    enum class JobPhase { Browsing, Transferring, AwaitingDecision, Publishing, Completed, Canceled, Failed };
    enum class Direction { Download, Upload };
    struct Request { ConnectionPlan connection; Direction direction=Direction::Download; std::wstring local; std::string remote; };
    // Authority is a live endpoint-owned object, not a remembered Replace boolean.
    // The endpoint must pin the exact target identity until Commit or abandonment.
    struct Replacement { virtual ~Replacement()=default; virtual void* Query(const void*) noexcept {return nullptr;} };
    struct Conflict : std::runtime_error
    {
        std::shared_ptr<Replacement> target;
        explicit Conflict(std::shared_ptr<Replacement> value) : std::runtime_error("Destination exists; choose replacement or a new name"),target(std::move(value)) {}
    };
    struct RetainedArtifact {std::wstring local;std::string remote;std::string detail;};
    struct PublishedWithCleanup : std::runtime_error
    {
        std::vector<RetainedArtifact> retained;
        PublishedWithCleanup(std::string message,std::vector<RetainedArtifact> files) : std::runtime_error(std::move(message)),retained(std::move(files)){}
    };
    struct JobSnapshot
    {
        uint64_t id=0, revision=0;
        JobPhase phase=JobPhase::Transferring;
        Request request;
        Progress progress{};
        std::string error;
        uint64_t decision=0;
        bool published=false,publicationUncertain=false;std::vector<RetainedArtifact> retained,shutdownContext;
    };
    inline std::wstring DisplayJob(const JobSnapshot& job)
    {
        const auto widen=[](std::string_view value){if(value.empty())return std::wstring{};const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0);if(!count)throw std::runtime_error("Invalid UTF-8 job detail");std::wstring output(static_cast<size_t>(count),0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),output.data(),count);return output;};
        const auto phase=job.phase==JobPhase::Completed?L"Completed":job.phase==JobPhase::Canceled?L"Canceled":job.phase==JobPhase::Failed?L"Failed":job.phase==JobPhase::AwaitingDecision?L"Destination conflict":job.phase==JobPhase::Publishing?L"Publishing":L"Transferring";
        auto result=std::wstring(phase)+L" - "+std::to_wstring(job.progress.completed)+L" / "+std::to_wstring(job.progress.total)+L" bytes\nLocal: "+job.request.local+L"\nRemote: "+widen(job.request.remote);
        if(job.publicationUncertain)result+=L"\nPublication outcome unconfirmed - inspect destination before retrying.";
        if(!job.error.empty())result+=L"\n"+widen(job.error);
        for(const auto& file:job.retained)result+=L"\n"+widen(file.detail)+L": "+(file.local.empty()?widen(file.remote):file.local);
        return result;
    }
    struct BrowserSnapshot { uint64_t generation=0; std::string path,error; std::vector<Entry> entries; bool busy=false; ConnectionPlan connection; };
    // Session implementation provides the protocol and physical publication boundary.
    // Cancellation is thread-safe and only owns this session's child/resources.
    struct Session
    {
        virtual ~Session()=default;
        virtual std::vector<Entry> Browse(std::string_view path)=0;
        virtual void Prepare(const Request&,const std::shared_ptr<Replacement>&)=0;
        virtual void Transfer(const Request&,const ProgressCallback&,const CancelCheck&)=0;
        virtual void Publish(const Request&,const std::shared_ptr<Replacement>&)=0;
        virtual void Cancel() noexcept=0;
        virtual std::vector<RetainedArtifact> Retained()const{return {};}
        virtual std::vector<RetainedArtifact> ShutdownContext()const{return Retained();}
    };
    class Service
    {
    public:
        using Factory=std::function<std::shared_ptr<Session>(const ConnectionPlan&)>;
        explicit Service(Factory factory) : _factory(std::move(factory)) {}
        ~Service()
        {
            BeginShutdown();
            if(_worker.joinable())_worker.join();
        }
        Service(const Service&)=delete;
        // Process-owned snapshots survive view destruction. Views subscribe with a
        // weak UI callback and unsubscribe; the service never retains a view/model.
        uint64_t Subscribe(std::function<void()> callback) {std::scoped_lock lock(_mutex);const auto token=++_subscription;_observers.emplace_back(token,std::move(callback));return token;}
        void Unsubscribe(uint64_t token) {std::scoped_lock lock(_mutex);std::erase_if(_observers,[&](const auto& o){return o.first==token;});}
        std::vector<JobSnapshot> Jobs() const {std::scoped_lock lock(_mutex);return _jobs;}
        BrowserSnapshot Browser() const {std::scoped_lock lock(_mutex);return _browser;}
        void Enabled(bool enabled) {std::scoped_lock lock(_mutex);_enabled=enabled;} // Never cancels an existing operation.
        bool HasPendingWork()const{std::scoped_lock lock(_mutex);return _busy||static_cast<bool>(_pendingTarget);}
        void BeginShutdown(bool notify=true)
        {
            {std::scoped_lock lock(_mutex);_stopping=true;_enabled=false;
                bool publishing=false;for(auto& job:_jobs){if(job.id==_activeJob&&job.phase==JobPhase::Publishing)publishing=true;if(job.phase==JobPhase::AwaitingDecision){job.phase=JobPhase::Canceled;job.decision=0;job.error="Canceled during application exit";++job.revision;}}
                _cancel=!publishing;if(_session&&!publishing)_session->Cancel();_pendingTarget.reset();
            }if(notify)Notify();
        }
        // Called through the owning DLL. Neither method waits for a held lock.
        bool TryDrainComplete()
        {
            std::unique_lock entry(_entry,std::try_to_lock);if(!entry.owns_lock())return false;
            {std::unique_lock lock(_mutex,std::try_to_lock);if(!lock.owns_lock()||_busy||!_stopping)return false;}
            if(_worker.joinable()){if(WaitForSingleObject(_worker.native_handle(),0)!=WAIT_OBJECT_0)return false;_worker.join();}return true;
        }
        void MarkShutdownTimeout()
        {
            std::unique_lock lock(_mutex,std::try_to_lock);if(!lock.owns_lock())return;
            for(auto& job:_jobs)if(job.id==_activeJob){if(job.phase==JobPhase::Publishing)job.publicationUncertain=true;job.error="Shutdown deadline exceeded; completion/cleanup not confirmed";++job.revision;}
        }
        std::wstring ShutdownStatus()const
        {
            std::unique_lock lock(_mutex,std::try_to_lock);if(!lock.owns_lock())return L"Worker cleanup is busy; outcome is not yet confirmed.";
            if(!_busy)return L"No transfer operation remains active; waiting for worker/capture cleanup.";
            if(!_activeJob)return L"Canceling SFTP browsing; waiting for its owned process to exit.";
            for(const auto& job:_jobs)if(job.id==_activeJob){auto text=DisplayJob(job);for(const auto& file:job.shutdownContext)text+=L"\nPossible incomplete artifact: "+(file.local.empty()?WideRemote(file.remote):file.local);return text;}
            return L"Transfer outcome is not yet confirmed.";
        }
        bool Busy() const {std::scoped_lock lock(_mutex);return _busy;}
        uint64_t Browse(ConnectionPlan connection,std::string path)
        {
            ValidatePath(path);std::unique_lock entry(_entry);
            uint64_t generation;
            {std::scoped_lock lock(_mutex);RequireStart();generation=++_browser.generation;_browser={generation,path,{}, {},true,connection};_busy=true;_cancel=false;}
            JoinPrevious();
            Launch([this,connection=std::move(connection),path=std::move(path),generation]{
                try {auto session=Connect(connection);CheckCancel();auto entries=session->Browse(path);CheckCancel();std::scoped_lock lock(_mutex);if(generation==_browser.generation)_browser.entries=std::move(entries);}
                catch(const std::exception& error){std::scoped_lock lock(_mutex);if(generation==_browser.generation)_browser.error=_cancel?"Canceled":error.what();}
                FinishBrowse(generation);
            });entry.unlock();Notify();return generation;
        }
        uint64_t Submit(Request request)
        {
            ValidatePath(request.remote);if(request.local.empty())throw std::invalid_argument("Choose a local file");
            std::unique_lock entry(_entry);uint64_t id;
            {std::scoped_lock lock(_mutex);RequireStart();id=++_nextJob;JobSnapshot job;job.id=id;job.request=std::move(request);_jobs.push_back(std::move(job));_busy=true;_cancel=false;}
            JoinPrevious();StartJob(id,{});entry.unlock();Notify();return id;
        }
        // One deliberate decision applies to this exact job and endpoint-held target.
        // Neither stale UI callbacks nor Retry can replay overwrite authority.
        bool Replace(uint64_t id,uint64_t decision)
        {
            std::unique_lock entry(_entry);std::shared_ptr<Replacement> target;
            {std::scoped_lock lock(_mutex);if(_busy||!_enabled||_stopping||(_worker.joinable()&&_worker.get_id()==std::this_thread::get_id()))return false;auto& job=Find(id);if(job.phase!=JobPhase::AwaitingDecision||!decision||job.decision!=decision||_pendingJob!=id||!_pendingTarget)return false;target=std::move(_pendingTarget);job.decision=0;job.phase=JobPhase::Transferring;++job.revision;_busy=true;_cancel=false;}
            JoinPrevious();StartJob(id,std::move(target));entry.unlock();Notify();return true;
        }
        uint64_t Retry(uint64_t id)
        {
            Request request;
            {std::scoped_lock lock(_mutex);const auto& job=Find(id);if(job.phase!=JobPhase::Failed&&job.phase!=JobPhase::Canceled)throw std::runtime_error("Retry requires a failed or canceled job");if(job.published||job.publicationUncertain||std::any_of(job.retained.begin(),job.retained.end(),[](const auto& file){return !file.local.empty();}))throw std::runtime_error("Destination changed; review the published/retained files before starting a new transfer");request=job.request;}
            return Submit(std::move(request));
        }
        bool Cancel(uint64_t id)
        {
            {std::scoped_lock lock(_mutex);auto& job=Find(id);
                if(job.phase==JobPhase::Publishing||job.phase==JobPhase::Completed||job.phase==JobPhase::Canceled||job.phase==JobPhase::Failed)return false;
                if(job.phase==JobPhase::AwaitingDecision){job.phase=JobPhase::Canceled;job.decision=0;++job.revision;if(_pendingJob==id)_pendingTarget.reset();}
                else if(_activeJob==id){_cancel=true;if(_session)_session->Cancel();}else return false;
            }Notify();return true;
        }
        void CancelBrowse()
        {
            {std::scoped_lock lock(_mutex);if(!_browser.busy)return;_cancel=true;++_browser.generation;_browser.busy=false;_browser.error="Canceled";if(_session)_session->Cancel();}Notify();
        }
    private:
        static std::wstring WideRemote(std::string_view value)
        {
            const auto size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0);if(!size)return L"[invalid UTF-8 artifact path]";std::wstring text(static_cast<size_t>(size),0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),text.data(),size);return text;
        }
        Factory _factory;mutable std::mutex _mutex;std::mutex _entry;std::thread _worker;
        std::shared_ptr<Session> _session;std::shared_ptr<Replacement> _pendingTarget;
        uint64_t _nextJob=0,_nextDecision=0,_pendingJob=0,_activeJob=0,_subscription=0;
        bool _enabled=false,_stopping=false,_busy=false,_cancel=false;
        std::vector<JobSnapshot> _jobs;BrowserSnapshot _browser;
        std::vector<std::pair<uint64_t,std::function<void()>>> _observers;
        void RequireStart() const {if(_worker.joinable()&&_worker.get_id()==std::this_thread::get_id())throw std::runtime_error("Dispatch new operations outside the worker notification");if(!_enabled||_stopping)throw std::runtime_error("SFTP is disabled");if(_busy)throw std::runtime_error("Wait for or cancel the current SFTP operation");if(_jobs.size()>=1000)throw std::runtime_error("Preview transfer history limit reached; finish active work and restart Backslash");if(_pendingTarget)throw std::runtime_error("Resolve the pending destination conflict first");}
        JobSnapshot& Find(uint64_t id){for(auto& job:_jobs)if(job.id==id)return job;throw std::invalid_argument("Unknown transfer job");}
        const JobSnapshot& Find(uint64_t id)const{for(const auto& job:_jobs)if(job.id==id)return job;throw std::invalid_argument("Unknown transfer job");}
        void CheckCancel(){std::scoped_lock lock(_mutex);if(_cancel||_stopping)throw std::runtime_error("Canceled");}
        void JoinPrevious(){if(_worker.joinable())_worker.join();}
        std::shared_ptr<Session> Connect(const ConnectionPlan& plan)
        {
            auto session=_factory(plan);if(!session)throw std::runtime_error("SFTP transport unavailable");
            {std::scoped_lock lock(_mutex);_session=session;if(_cancel||_stopping)session->Cancel();}return session;
        }
        void FinishBrowse(uint64_t generation)
        {
            {std::scoped_lock lock(_mutex);if(generation==_browser.generation)_browser.busy=false;_session.reset();_busy=false;}Notify();
        }
        void StartJob(uint64_t id,std::shared_ptr<Replacement> target)
        {
            {std::scoped_lock lock(_mutex);_activeJob=id;}
            Launch([this,id,target=std::move(target)]{
                try {
                    Request request;{std::scoped_lock lock(_mutex);request=Find(id).request;}
                    auto session=Connect(request.connection);CheckCancel();session->Prepare(request,target);CheckCancel();
                    session->Transfer(request,[this,id,last=ULONGLONG{0}](Progress p)mutable{{std::scoped_lock lock(_mutex);auto& job=Find(id);job.progress=p;++job.revision;}const auto now=GetTickCount64();if(p.completed==p.total||now-last>=100){last=now;Notify();}},[this]{CheckCancel();});
                    {std::scoped_lock lock(_mutex);if(_cancel||_stopping)throw std::runtime_error("Canceled");auto& job=Find(id);job.shutdownContext=session->ShutdownContext();job.phase=JobPhase::Publishing;++job.revision;}
                    Notify(); // From this point Cancel returns false. Publication outcome wins.
                    session->Publish(request,target);
                    {std::scoped_lock lock(_mutex);auto& job=Find(id);job.phase=JobPhase::Completed;job.published=true;job.publicationUncertain=false;job.shutdownContext.clear();job.error.clear();++job.revision;}
                } catch(const PublishedWithCleanup& warning){std::scoped_lock lock(_mutex);auto& job=Find(id);job.phase=JobPhase::Completed;job.published=true;job.publicationUncertain=false;job.shutdownContext.clear();job.retained=warning.retained;job.error=warning.what();++job.revision;}
                catch(const Conflict& conflict){std::scoped_lock lock(_mutex);auto& job=Find(id);if(_cancel||_stopping){job.phase=JobPhase::Canceled;}else if(conflict.target){job.phase=JobPhase::AwaitingDecision;job.decision=++_nextDecision;_pendingTarget=conflict.target;_pendingJob=id;}else{job.phase=JobPhase::Failed;}job.error=conflict.what();++job.revision;}
                catch(const std::exception& error){std::scoped_lock lock(_mutex);auto& job=Find(id);if(job.phase==JobPhase::Publishing)job.publicationUncertain=true;job.phase=_cancel?JobPhase::Canceled:JobPhase::Failed;job.error=(job.publicationUncertain?std::string("Publication result unconfirmed; "):(_cancel?std::string("Canceled; "):std::string{}))+error.what();if(_session)job.retained=_session->Retained();++job.revision;}
                {std::scoped_lock lock(_mutex);_session.reset();_busy=false;_activeJob=0;}Notify();
            });
        }
        template<typename Task> void Launch(Task&& task)
        {
            try{_worker=std::thread(std::forward<Task>(task));}
            catch(...){
                {std::scoped_lock lock(_mutex);if(_activeJob){auto& job=Find(_activeJob);job.phase=JobPhase::Failed;job.error="Cannot start SFTP worker";++job.revision;}else{_browser.busy=false;_browser.error="Cannot start SFTP worker";}_busy=false;_activeJob=0;}
                Notify();throw;
            }
        }
        void Notify() noexcept
        {
            std::vector<std::function<void()>> callbacks;try{{std::scoped_lock lock(_mutex);for(const auto& o:_observers)callbacks.push_back(o.second);}for(auto& callback:callbacks)try{callback();}catch(...){}}catch(...){}
        }
    };
}
