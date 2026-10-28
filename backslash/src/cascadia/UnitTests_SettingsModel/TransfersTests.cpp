// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "pch.h"
#include <thread>
#include "../inc/TransferJobs.h"
#include "../inc/SettingsPlacementGeometry.h"
#include "../inc/SettingsClosePolicy.h"
#include "../TerminalApp/SettingsViewHost.h"
#include "../TerminalSettingsModel/GlobalAppSettings.h"
#include "../TerminalSettingsModel/TerminalSettingsSerializationHelpers.h"
using namespace WEX::TestExecution;
using namespace winrt::Microsoft::Terminal::Settings::Model;
namespace T = ::Microsoft::Terminal::Backslash::Transfers;
namespace B = ::Microsoft::Terminal::Backslash;
namespace SettingsModelUnitTests
{
    class TransfersTests
    {
        BEGIN_TEST_CLASS(TransfersTests)
        END_TEST_CLASS()
        TEST_METHOD(SearchAndStatusCombine);
        TEST_METHOD(ActiveIncludesPendingAndCancellation);
        TEST_METHOD(JobSnapshotsAndObservers);
        TEST_METHOD(ConcurrentSubmissionKeepsDistinctJobs);
        TEST_METHOD(ViewOwnershipDoesNotOwnJobs);
        TEST_METHOD(PlacementPreferenceIsIndependent);
        TEST_METHOD(IdentityAndPlacementSafety);
    };
    void TransfersTests::SearchAndStatusCombine()
    {
        const T::Job failed{L"id",L"résumé.zip",L"C:\\Work\\Ångström",L"sftp://example.org/incoming",T::Status::Failed,L"Permission denied"};
        VERIFY_IS_TRUE(T::Matches(failed,T::Filter::All,L""));
        VERIFY_IS_TRUE(T::Matches(failed,T::Filter::Failed,L"RÉSUMÉ"));
        VERIFY_IS_TRUE(T::Matches(failed,T::Filter::Failed,L"ångström"));
        VERIFY_IS_TRUE(T::Matches(failed,T::Filter::Failed,L"EXAMPLE.ORG"));
        VERIFY_IS_TRUE(T::Matches(failed,T::Filter::Failed,L"denied"));
        VERIFY_IS_FALSE(T::Matches(failed,T::Filter::Active,L"résumé"));
        VERIFY_IS_FALSE(T::Matches(failed,T::Filter::Completed,L"résumé"));
        VERIFY_IS_FALSE(T::Matches(failed,T::Filter::Canceled,L"résumé"));
        VERIFY_IS_FALSE(T::Matches(failed,T::Filter::Failed,L"other-file"));
        VERIFY_IS_FALSE(T::Contains(L"",L"query"));
        VERIFY_IS_FALSE(T::Contains(L"a",L"longer"));
    }
    void TransfersTests::ActiveIncludesPendingAndCancellation()
    {
        for (const auto status : {T::Status::Queued,T::Status::Connecting,T::Status::Running,T::Status::Verifying,T::Status::Canceling})
        {
            const T::Job job{L"id",L"file",L"local",L"remote",status,L""};
            VERIFY_IS_TRUE(T::Matches(job,T::Filter::Active,L"FILE"));
            VERIFY_IS_FALSE(T::Matches(job,T::Filter::Canceled,L""));
        }
        for (const auto status : {T::Status::Completed,T::Status::Failed,T::Status::Canceled})
        {
            VERIFY_IS_FALSE(T::IsActive(status));
        }
        VERIFY_IS_TRUE(T::Matches(T::Job{L"a",L"",L"",L"",T::Status::Completed,L""},T::Filter::Completed,L""));
        VERIFY_IS_TRUE(T::Matches(T::Job{L"b",L"",L"",L"",T::Status::Canceled,L""},T::Filter::Canceled,L""));
    }
    void TransfersTests::JobSnapshotsAndObservers()
    {
        T::Store store;
        VERIFY_IS_TRUE(store.Snapshot().empty());
        VERIFY_IS_FALSE(store.Upsert({}));
        int calls = 0;
        const auto observer = store.Subscribe([&] { ++calls; VERIFY_IS_FALSE(store.Snapshot().empty()); }); // Callbacks must run outside the lock.
        VERIFY_IS_TRUE(store.Upsert({L"id",L"file",L"local",L"remote",T::Status::Running,L""}));
        auto snapshot = store.Snapshot();
        VERIFY_ARE_EQUAL(size_t{1},snapshot.size());
        snapshot[0].name=L"modified snapshot";
        VERIFY_ARE_EQUAL(std::wstring{L"file"},store.Snapshot()[0].name);
        VERIFY_IS_TRUE(store.Upsert({L"id",L"file",L"local",L"remote",T::Status::Completed,L""}));
        VERIFY_ARE_EQUAL(size_t{1},store.Snapshot().size());
        VERIFY_ARE_EQUAL(2,calls);
        store.Unsubscribe(observer);
        VERIFY_IS_TRUE(store.Upsert({L"second",L"next",L"local",L"remote",T::Status::Failed,L""}));
        VERIFY_ARE_EQUAL(2,calls);
        VERIFY_ARE_EQUAL(size_t{2},store.Snapshot().size());
    }
    void TransfersTests::ConcurrentSubmissionKeepsDistinctJobs()
    {
        T::Store store;
        std::vector<std::jthread> publishers;
        for (int publisher=0; publisher<4; ++publisher)
        {
            publishers.emplace_back([&,publisher] {
                for (int index=0; index<25; ++index)
                {
                    const auto id=std::to_wstring(publisher*25+index);
                    store.Upsert({id,L"file",L"local",L"remote",T::Status::Running,L""});
                    store.Upsert({id,L"file",L"local",L"remote",T::Status::Completed,L""});
                }
            });
        }
        publishers.clear(); // jthread joins before inspecting the complete snapshot.
        const auto jobs=store.Snapshot();
        VERIFY_ARE_EQUAL(size_t{100},jobs.size());
        for (const auto& job : jobs) { VERIFY_IS_TRUE(job.status==T::Status::Completed); }
    }
    void TransfersTests::ViewOwnershipDoesNotOwnJobs()
    {
        T::Store store; // Future submission publishes directly to this owner, before a view exists.
        store.Upsert({L"running",L"file",L"local",L"remote",T::Status::Running,L""});
        using Host=winrt::TerminalApp::implementation::SettingsViewHost;
        Host viewHost;
        const auto lease=viewHost.Request();
        VERIFY_IS_TRUE(viewHost.ClaimCreation(lease));
        bool focused=false;
        VERIFY_IS_TRUE(viewHost.Ready(lease,[&] { focused=true; }));
        VERIFY_ARE_EQUAL(uint64_t{0},viewHost.Request()); VERIFY_IS_TRUE(focused);
        const auto subscription=store.Subscribe([] {});
        // Exact view-close responsibilities: unsubscribe and release UI ownership.
        store.Unsubscribe(subscription); viewHost.Release(lease);
        const auto reopened=viewHost.Request();
        VERIFY_IS_TRUE(reopened>lease);
        VERIFY_ARE_EQUAL(size_t{1},store.Snapshot().size());
        VERIFY_IS_TRUE(store.Snapshot()[0].status==T::Status::Running);
        viewHost.Release(reopened);
    }
    void TransfersTests::PlacementPreferenceIsIndependent()
    {
        namespace I=winrt::Microsoft::Terminal::Settings::Model::implementation;
        auto settings=winrt::make_self<I::GlobalAppSettings>();
        VERIFY_ARE_EQUAL(SettingsLaunchMode::Tab,settings->TransfersLaunchMode());
        for (const auto& item : {std::pair{"tab",SettingsLaunchMode::Tab},std::pair{"sideBySide",SettingsLaunchMode::SideBySide},std::pair{"separateWindow",SettingsLaunchMode::SeparateWindow}})
        {
            Json::Value json{Json::objectValue};
            json["backslash.transfersLaunchMode"]=item.first;
            json["backslash.settingsLaunchMode"]="separateWindow";
            settings=I::GlobalAppSettings::FromJson(json);
            VERIFY_ARE_EQUAL(item.second,settings->TransfersLaunchMode());
            VERIFY_ARE_EQUAL(SettingsLaunchMode::SeparateWindow,settings->SettingsLaunchMode());
            VERIFY_ARE_EQUAL(std::string{item.first},settings->ToJson()["backslash.transfersLaunchMode"].asString());
            VERIFY_ARE_EQUAL(item.second,settings->Copy()->TransfersLaunchMode());
        }
    }
    void TransfersTests::IdentityAndPlacementSafety()
    {
        using Host=winrt::TerminalApp::implementation::SettingsViewHost;
        auto& settings=Host::Instance();
        auto& transfers=Host::Instance(B::BuiltinViewId::Transfers);
        VERIFY_IS_TRUE(&settings!=&transfers);
        VERIFY_ARE_EQUAL(0u,static_cast<uint32_t>(B::BuiltinViewId::Settings));
        VERIFY_ARE_EQUAL(1u,static_cast<uint32_t>(B::BuiltinViewId::Transfers));
        const auto settingsLease=settings.Request(); const auto transfersLease=transfers.Request();
        VERIFY_IS_TRUE(settingsLease!=0); VERIFY_IS_TRUE(transfersLease!=0);
        transfers.Release(transfersLease);
        VERIFY_ARE_EQUAL(settingsLease,settings.Pending());
        settings.Release(settingsLease);
        VERIFY_IS_FALSE(B::SettingsFitsRightSplit(600,800,360,300));
        VERIFY_IS_FALSE(B::SettingsFitsRightSplit(1000,303,360,300));
        VERIFY_IS_TRUE(B::SettingsFitsRightSplit(1000,400,360,300));
        VERIFY_IS_TRUE(B::SettingsClosePolicy::ProtectTabClose(true,2));
        VERIFY_IS_FALSE(B::SettingsClosePolicy::ProtectTabClose(true,1));
    }
}
