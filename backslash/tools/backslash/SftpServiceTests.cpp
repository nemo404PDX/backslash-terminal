// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "../../src/cascadia/inc/SftpNativeSession.h"
#include <iostream>
#include <future>
using namespace Microsoft::Terminal::Backslash::Transfers::Sftp;
void Require(bool value){if(!value)throw std::runtime_error("Service assertion failed");}
struct ExactTarget:Replacement {uint64_t identity=42;};
struct Fake:Session
{
    std::atomic<bool> canceled=false,hold=false,published=false,entered=false,publishHold=false,publishEntered=false;
    bool conflict=false,fail=false,cleanupWarning=false;std::shared_ptr<Replacement> exact=std::make_shared<ExactTarget>();
    std::vector<Entry> Browse(std::string_view)override{entered=true;while(hold&&!canceled)Sleep(1);return {{"file",{}}};}
    void Prepare(const Request&,const std::shared_ptr<Replacement>& approval)override{if(conflict&&!approval)throw Conflict(exact);if(approval&&approval!=exact)throw std::runtime_error("Wrong target authority");}
    void Transfer(const Request&,const ProgressCallback& progress,const CancelCheck& check)override{entered=true;progress({1,2});while(hold&&!canceled){check();Sleep(1);}check();if(fail)throw std::runtime_error("Mock disconnect");progress({2,2});}
    void Publish(const Request&,const std::shared_ptr<Replacement>&)override{publishEntered=true;while(publishHold)Sleep(1);published=true;if(cleanupWarning)throw PublishedWithCleanup("Published; backup retained",{{L"D:\\owned-fixture\\backup",{},"Retirement failed"}});}
    void Cancel()noexcept override{canceled=true;}
};
void Wait(const std::function<bool()>& predicate){const auto start=GetTickCount64();while(!predicate()){if(GetTickCount64()-start>3000)throw std::runtime_error("Fixture deadline exceeded");Sleep(1);}}
int main()
{
    Request request;request.local=L"D:\\fake\\file";request.remote="/fake/file";
    auto fake=std::make_shared<Fake>();Service service([&](const auto&){return fake;});service.Enabled(true);
    auto first=service.Submit(request);Wait([&]{return !service.Busy();});Require(service.Jobs()[0].phase==JobPhase::Completed&&!service.Cancel(first)&&fake->published);
    fake=std::make_shared<Fake>();fake->publishHold=true;auto publishing=service.Submit(request);Wait([&]{return fake->publishEntered.load();});Require(!service.Cancel(publishing));fake->publishHold=false;Wait([&]{return !service.Busy();});Require(service.Jobs().back().phase==JobPhase::Completed);
    fake=std::make_shared<Fake>();fake->hold=true;auto canceled=service.Submit(request);Wait([&]{return fake->entered.load();});Require(service.Cancel(canceled));Wait([&]{return !service.Busy();});Require(service.Jobs().back().phase==JobPhase::Canceled&&!fake->published);
    fake=std::make_shared<Fake>();auto retried=service.Retry(canceled);Wait([&]{return !service.Busy();});Require(retried!=canceled&&service.Jobs().back().phase==JobPhase::Completed);
    fake=std::make_shared<Fake>();fake->conflict=true;auto conflicted=service.Submit(request);Wait([&]{return !service.Busy();});auto pending=service.Jobs().back();Require(pending.phase==JobPhase::AwaitingDecision&&!fake->published);Require(!service.Replace(conflicted,pending.decision+1));Require(service.Replace(conflicted,pending.decision));Wait([&]{return !service.Busy();});Require(service.Jobs().back().phase==JobPhase::Completed&&fake->published&&!service.Replace(conflicted,pending.decision));
    fake=std::make_shared<Fake>();fake->conflict=true;auto no=service.Submit(request);Wait([&]{return !service.Busy();});const auto token=service.Jobs().back().decision;Require(service.Cancel(no)&&!service.Replace(no,token));
    fake=std::make_shared<Fake>();fake->hold=true;const auto browse=service.Browse({},".");Wait([&]{return fake->entered.load();});service.CancelBrowse();Wait([&]{return !service.Busy();});Require(service.Browser().generation!=browse&&service.Browser().entries.empty());
    fake=std::make_shared<Fake>();fake->hold=true;service.Submit(request);Wait([&]{return fake->entered.load();});service.Enabled(false);fake->hold=false;Wait([&]{return !service.Busy();});Require(service.Jobs().back().phase==JobPhase::Completed);
    bool denied=false;try{service.Submit(request);}catch(const std::exception&){denied=true;}Require(denied);
    service.Enabled(true);fake=std::make_shared<Fake>();fake->fail=true;service.Submit(request);Wait([&]{return !service.Busy();});Require(service.Jobs().back().phase==JobPhase::Failed&&!fake->published);
    fake=std::make_shared<Fake>();fake->cleanupWarning=true;const auto cleaned=service.Submit(request);Wait([&]{return !service.Busy();});Require(service.Jobs().back().phase==JobPhase::Completed&&service.Jobs().back().published&&service.Jobs().back().retained.size()==1);bool retryDenied=false;try{service.Retry(cleaned);}catch(const std::exception&){retryDenied=true;}Require(retryDenied);
    std::atomic<uint32_t> notifications=0;const auto subscription=service.Subscribe([&]{++notifications;const auto snapshot=service.Jobs();});service.Unsubscribe(subscription);fake=std::make_shared<Fake>();service.Submit(request);Wait([&]{return !service.Busy();});Require(notifications==0);
    {auto shutdown=std::make_shared<Fake>();shutdown->hold=true;Service owned([shutdown](const auto&){return shutdown;});owned.Enabled(true);owned.Submit(request);Wait([&]{return shutdown->entered.load();});}
    {auto exitPublishing=std::make_shared<Fake>();exitPublishing->publishHold=true;Service exiting([exitPublishing](const auto&){return exitPublishing;});exiting.Enabled(true);exiting.Submit(request);Wait([&]{return exitPublishing->publishEntered.load();});Require(exiting.HasPendingWork());exiting.BeginShutdown();Require(!exitPublishing->canceled);exitPublishing->publishHold=false;Wait([&]{return !exiting.Busy();});Require(exiting.Jobs().back().published&&exiting.Jobs().back().phase==JobPhase::Completed);bool refused=false;try{exiting.Submit(request);}catch(const std::exception&){refused=true;}Require(refused);}
    {auto exitPending=std::make_shared<Fake>();exitPending->conflict=true;Service exiting([exitPending](const auto&){return exitPending;});exiting.Enabled(true);auto id=exiting.Submit(request);Wait([&]{return !exiting.Busy();});Require(exiting.HasPendingWork());auto decision=exiting.Jobs().back().decision;exiting.BeginShutdown();Require(!exiting.HasPendingWork()&&exiting.Jobs().back().phase==JobPhase::Canceled&&!exiting.Replace(id,decision));}
    std::cout<<"PASS: process-owned snapshots, completion/cancel publication boundary, retry new identity, single-use exact replacement authority, stale decision rejection, stale browse suppression, disabled new submissions, failure, unsubscribe and active shutdown. Mock sessions only.\n";
}
