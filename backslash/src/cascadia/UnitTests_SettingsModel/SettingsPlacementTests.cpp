// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "pch.h"
#include "../inc/SettingsViewLifecycle.h"
#include "../inc/SettingsClosePolicy.h"
#include "../inc/FocusedPaneClose.h"
#include "../inc/SettingsPlacementGeometry.h"
#include "../TerminalApp/SettingsViewHost.h"
#include "../TerminalSettingsModel/GlobalAppSettings.h"
#include "../TerminalSettingsModel/TerminalSettingsSerializationHelpers.h"
using namespace WEX::TestExecution;
using namespace winrt::Microsoft::Terminal::Settings::Model;
namespace B = ::Microsoft::Terminal::Backslash;
namespace SettingsModelUnitTests
{
    class SettingsPlacementTests
    {
        BEGIN_TEST_CLASS(SettingsPlacementTests)
        END_TEST_CLASS()
        TEST_METHOD(PreferenceDefaultAndRoundtrip);
        TEST_METHOD(FocusedCloseSelectsNestedLeafNotParent);
        TEST_METHOD(FocusedCloseUsesClickedTabHistory);
        TEST_METHOD(FocusedCloseWithoutLeafDoesNothing);
        TEST_METHOD(FocusedCloseRejectsReplacedTargets);
        TEST_METHOD(FocusedCloseGateCoversCollapseAndStaleCallbacks);
        TEST_METHOD(SettingsReopensAfterSurvivingTerminalIsClosed);
        TEST_METHOD(FooterActionsReflowAtMeasuredWidth);
        TEST_METHOD(BatchCloseValidatesExactLeafContents);
        TEST_METHOD(BatchCloseAllowsOwnPromotionButRejectsExternalChanges);
        TEST_METHOD(BatchCloseKeepsSelectedParentSubtree);

        TEST_METHOD(CloseSettingsProtectsUnsavedDraft);
        TEST_METHOD(CloseGuardTracksPaneContext);
        TEST_METHOD(ClosingSettingsReleasesOnlyItsOwnership);
        TEST_METHOD(InitialWindowFailureAndCancellationReleaseReservation);
        TEST_METHOD(NestedActiveLeafMustFitIncomingSettings);
        TEST_METHOD(ReservedIdentityDoesNotDependOnPlacement);
        TEST_METHOD(PendingCreationCoalescesAndRejectsStaleCallbacks);
        TEST_METHOD(FailedTransferKeepsOriginalOwner);
        TEST_METHOD(TransferAcknowledgementChangesLeaseNotIdentity);
    };
    void SettingsPlacementTests::PreferenceDefaultAndRoundtrip()
    {
        namespace I = winrt::Microsoft::Terminal::Settings::Model::implementation;
        auto settings = winrt::make_self<I::GlobalAppSettings>();
        VERIFY_ARE_EQUAL(SettingsLaunchMode::Tab, settings->SettingsLaunchMode());
        for (const auto& item : { std::pair{"tab", SettingsLaunchMode::Tab}, std::pair{"sideBySide", SettingsLaunchMode::SideBySide}, std::pair{"separateWindow", SettingsLaunchMode::SeparateWindow} })
        {
            Json::Value json{Json::objectValue};json["backslash.settingsLaunchMode"] = item.first;
            settings = I::GlobalAppSettings::FromJson(json);
            VERIFY_ARE_EQUAL(item.second, settings->SettingsLaunchMode());
            VERIFY_ARE_EQUAL(std::string{item.first}, settings->ToJson()["backslash.settingsLaunchMode"].asString());
            VERIFY_ARE_EQUAL(item.second, settings->Copy()->SettingsLaunchMode());
        }
    }
    void SettingsPlacementTests::ReservedIdentityDoesNotDependOnPlacement()
    {
        VERIFY_ARE_EQUAL(0u, static_cast<uint32_t>(B::BuiltinViewId::Settings));
        VERIFY_IS_TRUE(B::IsBuiltinSlot(9));VERIFY_IS_FALSE(B::IsBuiltinSlot(10));VERIFY_IS_TRUE(B::IsBuiltinSlot(19,20));
    }
    void SettingsPlacementTests::PendingCreationCoalescesAndRejectsStaleCallbacks()
    {
        B::SettingsViewLifecycle state;const auto first=state.Open();
        VERIFY_IS_TRUE(first.action==B::SettingsViewLifecycle::Action::Create);
        VERIFY_IS_TRUE(state.Open().action==B::SettingsViewLifecycle::Action::Wait);
        VERIFY_IS_FALSE(state.Ready(first.lease+1));VERIFY_IS_TRUE(state.Ready(first.lease));
        VERIFY_IS_TRUE(state.Open().action==B::SettingsViewLifecycle::Action::Focus);
        VERIFY_IS_TRUE(state.Close(first.lease));const auto second=state.Open();VERIFY_IS_FALSE(state.Close(first.lease));
        VERIFY_IS_TRUE(state.Failed(second.lease));VERIFY_IS_TRUE(state.Open().action==B::SettingsViewLifecycle::Action::Create);
    }
    void SettingsPlacementTests::FailedTransferKeepsOriginalOwner()
    {
        B::SettingsViewLifecycle state;auto owner=state.Open().lease;state.Ready(owner);auto target=state.BeginTransfer(owner);
        VERIFY_IS_FALSE(state.Close(owner));VERIFY_IS_FALSE(state.Failed(target+1));VERIFY_IS_TRUE(state.Failed(target));
        VERIFY_ARE_EQUAL(owner,state.Owner());VERIFY_IS_TRUE(state.Open().action==B::SettingsViewLifecycle::Action::Focus);
    }
    void SettingsPlacementTests::TransferAcknowledgementChangesLeaseNotIdentity()
    {
        B::SettingsViewLifecycle state;auto source=state.Open().lease;state.Ready(source);auto target=state.BeginTransfer(source);
        VERIFY_IS_TRUE(state.Ready(target));VERIFY_IS_FALSE(state.Close(source));VERIFY_ARE_EQUAL(target,state.Owner());
        VERIFY_IS_TRUE(state.Close(target));VERIFY_IS_TRUE(state.Open().lease>target);
    }
    void SettingsPlacementTests::InitialWindowFailureAndCancellationReleaseReservation()
    {
        using Host = winrt::TerminalApp::implementation::SettingsViewHost;
        Host host;
        const auto failed = host.Request();
        VERIFY_ARE_EQUAL(uint64_t{0}, host.Request());
        VERIFY_IS_TRUE(host.FailUnclaimedCreation(failed));
        const auto cancelled = host.Request();
        VERIFY_IS_TRUE(cancelled > failed);
        host.Release(cancelled); // Destination creation failure/cancel acknowledgement.
        const auto current = host.Request();
        VERIFY_IS_FALSE(host.ClaimCreation(failed)); // Late startup cannot claim newer reservation.
        VERIFY_IS_TRUE(host.ClaimCreation(current));
        VERIFY_IS_FALSE(host.ClaimCreation(current));
        VERIFY_IS_FALSE(host.FailUnclaimedCreation(current)); // Claimed content owns cleanup.
        bool focused = false;
        VERIFY_IS_TRUE(host.Ready(current, [&] { focused = true; }));
        VERIFY_IS_FALSE(host.FailUnclaimedCreation(failed)); // Stale timer cannot clear current owner.
        VERIFY_ARE_EQUAL(uint64_t{0}, host.Request()); VERIFY_IS_TRUE(focused);
        host.Release(current); VERIFY_IS_TRUE(host.Request() > current);
    }
    void SettingsPlacementTests::NestedActiveLeafMustFitIncomingSettings()
    {
        VERIFY_IS_FALSE(B::SettingsFitsRightSplit(600, 800, 480, 360)); // Two columns in a 1200-DIP tab.
        VERIFY_IS_FALSE(B::SettingsFitsRightSplit(1200, 300, 480, 360)); // Nested short row.
        VERIFY_IS_TRUE(B::SettingsFitsRightSplit(1000, 400, 480, 360));
        VERIFY_IS_FALSE(B::SettingsFitsRightSplit(967, 400, 480, 360));
        VERIFY_IS_TRUE(B::SettingsFitsRightSplit(968, 364, 480, 360));
        VERIFY_IS_FALSE(B::SettingsFitsRightSplit(968, 363, 480, 360));
        VERIFY_IS_FALSE(B::SettingsFitsRightSplit(0, 400, 480, 360));
    }

    void SettingsPlacementTests::CloseSettingsProtectsUnsavedDraft()
    {
        namespace P = B::SettingsClosePolicy;
        VERIFY_IS_TRUE(P::Evaluate(false,false,false,false) == P::Decision::Close);
        VERIFY_IS_TRUE(P::Evaluate(false,false,false,true) == P::Decision::ConfirmDiscard);
        VERIFY_IS_TRUE(P::Evaluate(false,false,true,false) == P::Decision::ConfirmDiscard);
        for (bool changed : {false,true})
        {
            VERIFY_IS_TRUE(P::Evaluate(true,false,false,changed) == P::Decision::Wait);
            VERIFY_IS_TRUE(P::Evaluate(false,true,false,changed) == P::Decision::Wait);
        }
    }
    void SettingsPlacementTests::CloseGuardTracksPaneContext()
    {
        namespace P = B::SettingsClosePolicy;
        VERIFY_IS_FALSE(P::ProtectTabClose(true,1)); VERIFY_IS_TRUE(P::ProtectTabClose(true,2));
        VERIFY_IS_FALSE(P::ProtectTabClose(false,2)); VERIFY_IS_FALSE(P::ProtectTabClose(true,0));
    }
    void SettingsPlacementTests::ClosingSettingsReleasesOnlyItsOwnership()
    {
        winrt::TerminalApp::implementation::SettingsViewHost host;
        const auto original = host.Request(); VERIFY_IS_TRUE(host.ClaimCreation(original));
        unsigned focusCount = 0; VERIFY_IS_TRUE(host.Ready(original, [&] { ++focusCount; }));
        VERIFY_ARE_EQUAL(uint64_t{0}, host.Request()); VERIFY_ARE_EQUAL(1u, focusCount);
        host.Release(original);
        const auto reopened = host.Request(); VERIFY_IS_TRUE(reopened > original);
        VERIFY_IS_TRUE(host.ClaimCreation(reopened)); VERIFY_IS_TRUE(host.Ready(reopened, [] {}));
        host.Release(original); VERIFY_ARE_EQUAL(reopened, host.Owner()); host.Release(reopened);
    }



    namespace
    {
        struct CloseNode : std::enable_shared_from_this<CloseNode>
        {
            int content = 0;
            std::vector<std::shared_ptr<CloseNode>> children;
            std::weak_ptr<CloseNode> active, history;
            int GetContent() const { return children.empty() ? content : 0; }
            int GetLeafPaneCount() const { if (children.empty()) { return 1; } int count=0;for(const auto& child:children){count+=child->GetLeafPaneCount();}return count; }
            std::shared_ptr<CloseNode> GetActivePane() const { return active.lock(); }
            std::shared_ptr<CloseNode> GetLastFocusedLeaf()
            {
                if (children.empty()) { return shared_from_this(); }
                const auto last=history.lock();
                if (!last || std::find(children.begin(),children.end(),last)==children.end()) { return {}; }
                return last->GetLastFocusedLeaf();
            }
            template<typename F> auto WalkTree(F f) -> decltype(f(shared_from_this()))
            {
                if constexpr (std::is_void_v<decltype(f(shared_from_this()))>)
                {
                    f(shared_from_this()); for (const auto& child : children) { child->WalkTree(f); }
                }
                else
                {
                    if (auto result = f(shared_from_this())) { return result; }
                    for (const auto& child : children) { if (auto result = child->WalkTree(f)) { return result; } }
                    return {};
                }
            }
        };
        auto CloseLeaf(int content) { auto p=std::make_shared<CloseNode>();p->content=content;return p; }
        auto CloseBranch(std::initializer_list<std::shared_ptr<CloseNode>> children) { auto p=std::make_shared<CloseNode>();p->children=children;return p; }
    }
    void SettingsPlacementTests::FocusedCloseSelectsNestedLeafNotParent()
    {
        auto a=CloseLeaf(1), b=CloseLeaf(2), settings=CloseLeaf(-1), transfers=CloseLeaf(-2);
        auto nested=CloseBranch({a,b});auto root=CloseBranch({settings,CloseBranch({nested,transfers})});
        root->active=nested; nested->history=b;
        VERIFY_IS_TRUE(B::FocusedCloseLeaf(root)==b);
        VERIFY_IS_TRUE(B::FocusedCloseLeaf(root)!=nested);
        root->active=settings;VERIFY_IS_TRUE(B::FocusedCloseLeaf(root)==settings);
        root->active=transfers;VERIFY_IS_TRUE(B::FocusedCloseLeaf(root)==transfers);
        auto ordinary=CloseBranch({a,b});ordinary->active=b;VERIFY_IS_TRUE(B::FocusedCloseLeaf(ordinary)==b);
        VERIFY_ARE_EQUAL(1,a->content); VERIFY_ARE_EQUAL(2,b->content); // Selection never mutates a sibling.
    }
    void SettingsPlacementTests::FocusedCloseUsesClickedTabHistory()
    {
        auto a=CloseLeaf(1), b=CloseLeaf(2), c=CloseLeaf(3), s=CloseLeaf(-1);
        auto clicked=CloseBranch({a,s});clicked->active=a;
        auto selectedElsewhere=CloseBranch({b,c});selectedElsewhere->active=c;
        VERIFY_IS_TRUE(B::FocusedCloseLeaf(clicked)==a);
        VERIFY_IS_TRUE(B::FocusedCloseLeaf(selectedElsewhere)==c);
        selectedElsewhere->active=b;VERIFY_IS_TRUE(B::FocusedCloseLeaf(clicked)==a);
        clicked->active=s;VERIFY_IS_TRUE(B::FocusedCloseLeaf(clicked)==s);
        auto single=CloseLeaf(1);single->active=single;VERIFY_IS_TRUE(B::FocusedCloseLeaf(single)==single);
    }
    void SettingsPlacementTests::FocusedCloseWithoutLeafDoesNothing()
    {
        std::shared_ptr<CloseNode> missing;VERIFY_IS_TRUE(!B::FocusedCloseLeaf(missing));
        auto a=CloseLeaf(1), s=CloseLeaf(-1);auto root=CloseBranch({a,s});
        VERIFY_IS_TRUE(!B::FocusedCloseLeaf(root));
        root->active=root;VERIFY_IS_TRUE(!B::FocusedCloseLeaf(root));
        auto foreign=CloseLeaf(99);root->history=foreign;VERIFY_IS_TRUE(!B::FocusedCloseLeaf(root));
        root->active=a;a->content=0;VERIFY_IS_TRUE(!B::FocusedCloseLeaf(root)); // Closing animation in flight.
    }
    void SettingsPlacementTests::FocusedCloseRejectsReplacedTargets()
    {
        auto a=CloseLeaf(1), s=CloseLeaf(-1);auto root=CloseBranch({a,s});root->active=a;
        const auto captured=B::FocusedCloseLeaf(root);const auto content=captured->content;
        root->active=s;VERIFY_IS_TRUE(B::CloseTargetMatches(root,root,captured,content)); // Focus alone cannot retarget the request.
        auto other=CloseBranch({a,s});VERIFY_IS_FALSE(B::CloseTargetMatches(other,root,captured,content));
        a->content=9;VERIFY_IS_FALSE(B::CloseTargetMatches(root,root,captured,content));a->content=content;
        root->children[0]=CloseLeaf(content);VERIFY_IS_FALSE(B::CloseTargetMatches(root,root,captured,content));
        root->children[0]=a;VERIFY_IS_TRUE(B::CloseTargetMatches(root,root,captured,content));
    }
    void SettingsPlacementTests::FocusedCloseGateCoversCollapseAndStaleCallbacks()
    {
        B::CloseRequestGate gate;const auto first=gate.Begin();VERIFY_IS_TRUE(first!=0);
        VERIFY_ARE_EQUAL(0ull,gate.Begin()); // Prompt and collapse share the reservation.
        gate.End(first+1);VERIFY_ARE_EQUAL(0ull,gate.Begin());
        gate.End(first);const auto next=gate.Begin();VERIFY_IS_TRUE(next>first);
        gate.End(first);VERIFY_ARE_EQUAL(0ull,gate.Begin()); // Late callback cannot release a replacement request.
        gate.End(next);const auto collapse=gate.Begin();VERIFY_IS_TRUE(collapse>next);
        bool attached=true;gate.TrackCollapse(collapse,[&] { return attached; });
        VERIFY_ARE_EQUAL(0ull,gate.Begin()); // Content may already be empty while its animation node is attached.
        attached=false;const auto recovered=gate.Begin();VERIFY_IS_TRUE(recovered>collapse);
        gate.End(collapse);VERIFY_ARE_EQUAL(0ull,gate.Begin()); // Missing/late ancestor-close acknowledgement is safe.
        gate.End(recovered);VERIFY_IS_TRUE(gate.Begin()>recovered);
    }
    void SettingsPlacementTests::SettingsReopensAfterSurvivingTerminalIsClosed()
    {
        using Host = winrt::TerminalApp::implementation::SettingsViewHost;
        Host host;
        for (unsigned cycle=0;cycle<25;++cycle)
        {
            const auto original=host.Request();VERIFY_IS_TRUE(host.ClaimCreation(original));
            unsigned focused=0;VERIFY_IS_TRUE(host.Ready(original,[&] { ++focused; }));
            // Terminal close retains the same Settings content and lease.
            VERIFY_ARE_EQUAL(uint64_t{0},host.Request());VERIFY_ARE_EQUAL(1u,focused);
            // User closes the standalone Settings tab, then opens it beside a new terminal.
            host.Release(original);const auto reopened=host.Request();VERIFY_IS_TRUE(reopened>original);
            VERIFY_IS_TRUE(host.ClaimCreation(reopened));VERIFY_IS_TRUE(host.Ready(reopened,[] {}));
            host.Release(original);VERIFY_ARE_EQUAL(reopened,host.Owner());host.Release(reopened);
            // Closing Settings directly while the terminal survives likewise frees the slot.
            const auto direct=host.Request();VERIFY_IS_TRUE(host.ClaimCreation(direct));
            VERIFY_IS_TRUE(host.Ready(direct,[] {}));host.Release(direct);VERIFY_ARE_EQUAL(uint64_t{0},host.Owner());
        }
    }
    void SettingsPlacementTests::FooterActionsReflowAtMeasuredWidth()
    {
        namespace P=B::SettingsClosePolicy;
        VERIFY_IS_FALSE(P::StackFooterActions(280,100,160));
        VERIFY_IS_FALSE(P::StackFooterActions(268,100,160));
        VERIFY_IS_TRUE(P::StackFooterActions(267,100,160));
        VERIFY_IS_TRUE(P::StackFooterActions(148,100,160)); // Narrow content column beside expanded sidebar.
        VERIFY_IS_TRUE(P::StackFooterActions(400,250,250)); // Localized/scaled labels.
        VERIFY_IS_FALSE(P::StackFooterActions(536.0/2.0,100,160)); // DIPs at 200% DPI.
        VERIFY_IS_TRUE(P::StackFooterActions(0,100,160));
        VERIFY_IS_TRUE(P::StackFooterActions(std::numeric_limits<double>::quiet_NaN(),100,160));
        VERIFY_IS_TRUE(P::StackFooterActions(std::numeric_limits<double>::infinity(),100,160));
    }
    void SettingsPlacementTests::BatchCloseValidatesExactLeafContents()
    {
        auto a=CloseLeaf(1), b=CloseLeaf(2), settings=CloseLeaf(-1);
        auto root=CloseBranch({a,CloseBranch({b,settings})});
        VERIFY_IS_TRUE(B::LeafContentsMatch(root,std::vector{1,2,-1}));
        b->content=1;VERIFY_IS_FALSE(B::LeafContentsMatch(root,std::vector{1,2,-1})); // Duplicate cannot hide a missing identity.
        b->content=0;VERIFY_IS_FALSE(B::LeafContentsMatch(root,std::vector{1,-1})); // Empty animation node remains attached.
        b->content=2;VERIFY_IS_FALSE(B::LeafContentsMatch(root,std::vector{1,-1}));
        VERIFY_IS_FALSE(B::LeafContentsMatch(std::shared_ptr<CloseNode>{},std::vector<int>{}));
    }
    void SettingsPlacementTests::BatchCloseAllowsOwnPromotionButRejectsExternalChanges()
    {
        auto a=CloseLeaf(1), b=CloseLeaf(2), settings=CloseLeaf(-1);
        auto parent=CloseBranch({a,b});auto root=CloseBranch({parent,settings});
        VERIFY_IS_TRUE(B::LeafContentsMatch(root,std::vector{1,2,-1}));
        // Closing a promotes b's same content into its parent object.
        parent->children.clear();parent->content=2;
        VERIFY_IS_TRUE(B::LeafContentsMatch(root,std::vector{2,-1}));
        VERIFY_IS_TRUE(B::FindContentLeaf(root,2)==parent);
        VERIFY_IS_FALSE(B::ContainsPaneNode(root,b));
        parent->content=9;VERIFY_IS_FALSE(B::LeafContentsMatch(root,std::vector{2,-1}));
        parent->content=2;root->children.push_back(CloseLeaf(3));VERIFY_IS_FALSE(B::LeafContentsMatch(root,std::vector{2,-1}));
        root->children.pop_back();root->children.erase(root->children.begin()+1);VERIFY_IS_FALSE(B::LeafContentsMatch(root,std::vector{2,-1}));
    }
    void SettingsPlacementTests::BatchCloseKeepsSelectedParentSubtree()
    {
        auto a=CloseLeaf(1), b=CloseLeaf(2), settings=CloseLeaf(-1), transfers=CloseLeaf(-2);
        auto selected=CloseBranch({a,b});auto root=CloseBranch({selected,CloseBranch({settings,transfers})});
        VERIFY_IS_TRUE(B::ContainsPaneNode(selected,a));VERIFY_IS_TRUE(B::ContainsPaneNode(selected,b));
        VERIFY_IS_FALSE(B::ContainsPaneNode(selected,settings));VERIFY_IS_FALSE(B::ContainsPaneNode(selected,transfers));
        VERIFY_IS_TRUE(B::ContainsPaneNode(root,settings));
        VERIFY_IS_FALSE(B::ContainsPaneNode(std::shared_ptr<CloseNode>{},settings));
    }
}
