// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "pch.h"
#include "../TerminalSettingsModel/PinnedDestinations.h"

using namespace WEX::TestExecution;
namespace P = Microsoft::Terminal::Backslash::Pins;

namespace SettingsModelUnitTests
{
    class PinnedDestinationTests
    {
        TEST_CLASS(PinnedDestinationTests);
        TEST_METHOD(RoundtripTypedDestinations);
        TEST_METHOD(CaptureRequiresReportedDirectory);
        TEST_METHOD(NavigationDoesNotRepoint);
        TEST_METHOD(IdentityIsNotProfileOrPathDedup);
        TEST_METHOD(RejectUnsupportedOrSecretFields);
        TEST_METHOD(RejectInvalidCatalogAndBounds);
        TEST_METHOD(ContextSpecificPathValidation);
        TEST_METHOD(CloseUnpinExitAndCancellation);
        TEST_METHOD(FailedRemovalPreservesCatalog);
        TEST_METHOD(IdempotentRemovalDoesNotRewrite);
        TEST_METHOD(ExactProfileAndTargetPreflight);
        TEST_METHOD(RemotePathsNeverUseLocalProbe);

        static P::Destination pin(P::Kind kind = P::Kind::Local)
        {
            P::Destination result{ "{11111111-1111-4111-8111-111111111111}", "{22222222-2222-4222-8222-222222222222}", "", kind, "Fixture profile", "fixture-machine", "C:\\fixture folder\\A" };
            if (kind == P::Kind::Wsl) { result.source = "Windows.Terminal.Wsl"; result.host = "FixtureDistro"; result.path = "/home/fixture/A"; }
            if (kind == P::Kind::SshWindows || kind == P::Kind::SshPosix) { result.source = "Windows.Terminal.SSH"; result.host = "fixture-alias"; result.path = kind == P::Kind::SshWindows ? "D:\\fixture\\A" : "/srv/fixture/A"; }
            return result;
        }

        static P::ProfileTarget target(const P::Destination& value)
        {
            return { value.profile, value.source, value.host, value.kind, true, true, true };
        }

        static Json::Value json(const P::Catalog& catalog)
        {
            Json::Value root;
            Json::CharReaderBuilder builder;
            const auto content = catalog.Write();
            std::unique_ptr<Json::CharReader> reader{ builder.newCharReader() };
            std::string errors;
            VERIFY_IS_TRUE(reader->parse(content.data(), content.data() + content.size(), &root, &errors));
            return root;
        }
    };

    void PinnedDestinationTests::RoundtripTypedDestinations()
    {
        for (const auto kind : { P::Kind::Local, P::Kind::Wsl, P::Kind::SshWindows, P::Kind::SshPosix })
        {
            P::Catalog catalog; catalog.Add(pin(kind));
            const auto restored = P::Catalog::Read(catalog.Write());
            VERIFY_ARE_EQUAL(size_t{ 1 }, restored.pins.size());
            VERIFY_IS_TRUE(restored.pins[0] == catalog.pins[0]);
            VERIFY_IS_TRUE(P::Identity(restored.pins[0]).find(restored.pins[0].path) != std::string::npos);
            VERIFY_IS_TRUE(P::Identity(restored.pins[0]).find(restored.pins[0].host) != std::string::npos);
        }
    }

    void PinnedDestinationTests::CaptureRequiresReportedDirectory()
    {
        VERIFY_THROWS(P::Capture(pin(), P::DirectoryOrigin::Unknown), std::exception);
        VERIFY_THROWS(P::Capture(pin(), P::DirectoryOrigin::ProfileSeed), std::exception);
        VERIFY_IS_TRUE(P::Capture(pin(), P::DirectoryOrigin::Reported) == pin());
        auto ssh = pin(P::Kind::SshPosix); ssh.path = "C:\\local-client-cwd";
        VERIFY_THROWS(P::Capture(ssh, P::DirectoryOrigin::Reported), std::exception);
        ssh = pin(P::Kind::Local); ssh.source = "Windows.Terminal.SSH";
        VERIFY_THROWS(P::Capture(ssh, P::DirectoryOrigin::Reported), std::exception);
    }

    void PinnedDestinationTests::NavigationDoesNotRepoint()
    {
        auto working = pin();
        P::Catalog catalog; catalog.Add(P::Capture(working, P::DirectoryOrigin::Reported));
        const auto frozen = catalog.Write();
        working.path = "C:\\fixture folder\\B";
        working.label = "dynamic title after navigation";
        VERIFY_ARE_EQUAL(frozen, catalog.Write());
        auto reopened = P::Resolve(catalog.pins[0], target(catalog.pins[0]), [](const auto& p) { return p == "C:\\fixture folder\\A"; });
        VERIFY_ARE_EQUAL(std::string{ "C:\\fixture folder\\A" }, reopened.path);
        VERIFY_ARE_EQUAL(std::string{ "Fixture profile" }, reopened.label);
    }

    void PinnedDestinationTests::IdentityIsNotProfileOrPathDedup()
    {
        P::Catalog catalog; const auto first = pin(); catalog.Add(first);
        auto second = first; second.id = "{33333333-3333-4333-8333-333333333333}";
        catalog.Add(second);
        VERIFY_ARE_EQUAL(size_t{ 2 }, P::Catalog::Read(catalog.Write()).pins.size());
        VERIFY_THROWS(catalog.Add(first), std::exception);
        catalog.Remove(first.id);
        VERIFY_ARE_EQUAL(size_t{ 1 }, catalog.pins.size());
        VERIFY_ARE_EQUAL(second.id, catalog.pins[0].id);
    }

    void PinnedDestinationTests::RejectUnsupportedOrSecretFields()
    {
        P::Catalog catalog; catalog.Add(pin());
        for (const auto* field : { "commandline", "environment", "password", "credentials", "scrollback", "windowId", "contentId", "layout", "liveCwd", "folderMode" })
        {
            auto root = json(catalog); root["pins"][0][field] = "must not persist";
            VERIFY_THROWS(P::Catalog::Read(Json::writeString(Json::StreamWriterBuilder{}, root)), std::exception);
        }
    }

    void PinnedDestinationTests::RejectInvalidCatalogAndBounds()
    {
        for (const auto* content : { "{}", "[]", "{\"version\":2,\"pins\":[]}", "{\"version\":1,\"pins\":[],\"pins\":[]}", "{\"version\":1,\"pins\":{}}", "{\"version\":1,\"pins\":[]} trailing" })
        {
            VERIFY_THROWS(P::Catalog::Read(content), std::exception);
        }
        VERIFY_THROWS(P::Catalog::Read(std::string(P::Catalog::MaxBytes + 1, ' ')), std::exception);
        auto value = pin(); value.id = "{00000000-0000-0000-0000-000000000000}";
        VERIFY_THROWS(P::Validate(value), std::exception);
        P::Catalog catalog; catalog.pins.assign(P::Catalog::MaxPins + 1, pin());
        VERIFY_THROWS(catalog.Write(), std::exception);
        catalog = {}; catalog.Add(pin());
        auto root = json(catalog); root["pins"].append(root["pins"][0]);
        VERIFY_THROWS(P::Catalog::Read(Json::writeString(Json::StreamWriterBuilder{}, root)), std::exception);
    }

    void PinnedDestinationTests::ContextSpecificPathValidation()
    {
        for (const auto& path : { std::string{}, std::string{ "relative" }, std::string{ "C:relative" }, std::string{ "C:\\%TEMP%" }, std::string{ "C:\\line\nnext" }, std::string{ "C:\\a\0b", 6 }, std::string{ "\\\\.\\pipe\\name" } })
        {
            auto value = pin(); value.path = path;
            VERIFY_THROWS(P::Validate(value), std::exception);
        }
        auto unc = pin(); unc.path = "\\\\fixture-server\\fixture-share\\A";
        VERIFY_IS_TRUE(P::Validate(unc) == unc);
        for (const auto* path : { "~", "relative", "C:\\local", "/home/%USER%", "/home/a\nnext" })
        {
            auto value = pin(P::Kind::Wsl); value.path = path;
            VERIFY_THROWS(P::Validate(value), std::exception);
        }
    }

    void PinnedDestinationTests::CloseUnpinExitAndCancellation()
    {
        VERIFY_IS_TRUE((P::Plan(P::Intent::Unpin, true) == P::Action{ true, false, true }));
        VERIFY_IS_TRUE((P::Plan(P::Intent::IndividualClose, true) == P::Action{ true, true, false }));
        VERIFY_IS_TRUE((P::Plan(P::Intent::IndividualClose, true, false) == P::Action{ true, false, false }));
        for (auto intent : { P::Intent::WindowClose, P::Intent::QuitAll }) { VERIFY_IS_TRUE((P::Plan(intent, true) == P::Action{ false, true, false })); }
        for (auto intent : { P::Intent::Transfer, P::Intent::NaturalExit, P::Intent::Recovery }) { VERIFY_IS_TRUE(P::Plan(intent, true) == P::Action{}); }
        for (auto intent : { P::Intent::Unpin, P::Intent::IndividualClose, P::Intent::WindowClose, P::Intent::QuitAll, P::Intent::BulkTabClose }) { VERIFY_IS_TRUE(P::Plan(intent, false) == P::Action{}); }
        VERIFY_THROWS(P::Plan(P::Intent::Unpin, true, false), std::exception);
        VERIFY_IS_TRUE((P::Plan(P::Intent::BulkTabClose, true) == P::Action{ false, true, false }));
        VERIFY_IS_TRUE((P::Plan(P::Intent::PaneOnlyClose, true) == P::Action{ false, true, false }));
    }

    void PinnedDestinationTests::FailedRemovalPreservesCatalog()
    {
        P::Catalog catalog; catalog.Add(pin()); const auto before = catalog.Write();
        size_t writes = 0;
        VERIFY_THROWS(P::RemoveAfterWrite(catalog, pin().id, P::Intent::IndividualClose, true, [&](const auto&) { ++writes; throw std::runtime_error("Injected write failure"); }), std::exception);
        VERIFY_ARE_EQUAL(size_t{ 1 }, writes);
        VERIFY_ARE_EQUAL(before, catalog.Write());
        for (auto intent : { P::Intent::IndividualClose, P::Intent::Unpin, P::Intent::WindowClose, P::Intent::QuitAll, P::Intent::Transfer })
        {
            VERIFY_IS_FALSE(P::RemoveAfterWrite(catalog, pin().id, intent, false, [&](const auto&) { ++writes; }));
        }
        VERIFY_ARE_EQUAL(size_t{ 1 }, writes); VERIFY_ARE_EQUAL(before, catalog.Write());
    }

    void PinnedDestinationTests::IdempotentRemovalDoesNotRewrite()
    {
        P::Catalog catalog; catalog.Add(pin()); size_t writes = 0;
        auto writer = [&](const auto& content) { ++writes; VERIFY_IS_TRUE(P::Catalog::Read(content).pins.empty()); };
        VERIFY_IS_TRUE(P::RemoveAfterWrite(catalog, pin().id, P::Intent::Unpin, true, writer));
        VERIFY_IS_FALSE(P::RemoveAfterWrite(catalog, pin().id, P::Intent::IndividualClose, true, writer));
        VERIFY_ARE_EQUAL(size_t{ 1 }, writes);
    }

    void PinnedDestinationTests::ExactProfileAndTargetPreflight()
    {
        const auto value = pin();
        auto exists = [](const auto&) { return true; };
        VERIFY_THROWS(P::Resolve(value, std::nullopt, exists), std::exception);
        for (size_t fault = 0; fault < 6; ++fault)
        {
            auto profile = target(value);
            switch (fault)
            {
            case 0: profile.active = false; break;
            case 1: profile.source = "replaced-source"; break;
            case 2: profile.host = "different-host"; break;
            case 3: profile.kind = P::Kind::Wsl; break;
            case 4: profile.elevationCompatible = false; break;
            case 5: profile.supportsFixedPath = false; break;
            }
            VERIFY_THROWS(P::Resolve(value, profile, exists), std::exception);
        }
        VERIFY_THROWS(P::Resolve(value, target(value), [](const auto&) { return false; }), std::exception);
        VERIFY_IS_TRUE(P::Resolve(value, target(value), exists) == value);
    }

    void PinnedDestinationTests::RemotePathsNeverUseLocalProbe()
    {
        size_t localProbes = 0;
        for (auto kind : { P::Kind::Wsl, P::Kind::SshWindows, P::Kind::SshPosix })
        {
            auto value = pin(kind);
            VERIFY_IS_TRUE(P::Resolve(value, target(value), [&](const auto&) { ++localProbes; return false; }) == value);
        }
        VERIFY_ARE_EQUAL(size_t{ 0 }, localProbes);
    }
}
