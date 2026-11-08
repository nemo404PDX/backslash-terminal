// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "pch.h"
#include "../TerminalSettingsModel/PinnedStore.h"
#include <atomic>
#include <fstream>
#include <mutex>
#include <thread>

using namespace WEX::TestExecution;
namespace P = Microsoft::Terminal::Backslash::Pins;
namespace SettingsModelUnitTests
{
    class PinnedStoreTests
    {
        TEST_CLASS(PinnedStoreTests);
        TEST_METHOD(ConcurrentWindowAddsAreAdditive);
        TEST_METHOD(ConcurrentClaimHasOneWinner);
        TEST_METHOD(WindowExitAndSurvivorAccess);
        TEST_METHOD(CrashAndPidReuseRecovery);
        TEST_METHOD(TransferFenceAndStaleActions);
        TEST_METHOD(FailedTransferRecovery);
        TEST_METHOD(WriteFailureLeavesDiskUnchanged);
        TEST_METHOD(SettingsAndCopiesRemainIndependent);
        TEST_METHOD(InvalidDataIsNotOverwritten);
        TEST_METHOD(ExpiredRecoveryKeepsFenceUntilAttached);
        TEST_METHOD(AcceptedCompletionWriteFailure);
        TEST_METHOD(ClosedRemovalRejectsNewClaim);

        static std::filesystem::path root()
        {
            return std::filesystem::temp_directory_path() / (L"Backslash-pin-store-" + std::wstring{ winrt::to_hstring(P::NewId()) });
        }
        static P::Owner owner(uint64_t window)
        {
            return { P::NewId(), 123, 456, window, window };
        }
        static P::Destination pin()
        {
            return { P::NewId(), "{22222222-2222-4222-8222-222222222222}", "", P::Kind::Local, "Fixture", "fixture-machine", "C:\\fixture\\A" };
        }
        static P::Life alive(const P::Owner&) { return P::Life::Alive; }
        static std::string read(const std::filesystem::path& p)
        {
            std::ifstream stream{ p, std::ios::binary };
            return { std::istreambuf_iterator<char>{ stream }, std::istreambuf_iterator<char>{} };
        }
        static void write(const std::filesystem::path& p, const std::string& content)
        {
            std::filesystem::create_directories(p.parent_path());
            std::ofstream stream{ p, std::ios::binary }; stream << content;
        }
    };

    void PinnedStoreTests::ConcurrentWindowAddsAreAdditive()
    {
        const auto directory = root();
        std::vector<std::thread> threads;
        std::vector<std::exception_ptr> failures; std::mutex errors;
        for (size_t i = 0; i < 16; ++i)
        {
            threads.emplace_back([&, i] {
                try { P::Store{ directory, alive }.Add(pin(), owner(i + 1)); }
                catch (...) { std::scoped_lock lock{ errors }; failures.push_back(std::current_exception()); }
            });
        }
        for (auto& thread : threads) { thread.join(); }
        for (const auto& failure : failures)
        {
            try { std::rethrow_exception(failure); }
            catch (const winrt::hresult_error& error) { WEX::Logging::Log::Comment(error.message().c_str()); }
            catch (const std::exception& error) { WEX::Logging::Log::Comment(winrt::to_hstring(error.what()).c_str()); }
            catch (...) { WEX::Logging::Log::Comment(L"Pinned-store worker threw a non-standard exception."); }
        }
        VERIFY_IS_TRUE(failures.empty());
        const auto state = P::Store{ directory, alive }.Get();
        VERIFY_ARE_EQUAL(size_t{ 16 }, state.catalog.pins.size());
        VERIFY_ARE_EQUAL(size_t{ 16 }, state.leases.size());
    }

    void PinnedStoreTests::ConcurrentClaimHasOneWinner()
    {
        const auto directory = root(); P::Store store{ directory, alive };
        const auto destination = pin(); const auto firstOwner = owner(1);
        const auto first = store.Add(destination, firstOwner); store.Release(destination.id, firstOwner, first.nonce);
        std::atomic<size_t> winners{ 0 }, failures{ 0 }; std::vector<std::thread> threads;
        for (size_t i = 0; i < 16; ++i)
        {
            threads.emplace_back([&, i] { try { if (P::Store{ directory, alive }.Claim(destination.id, owner(i + 2))) { ++winners; } } catch (...) { ++failures; } });
        }
        for (auto& thread : threads) { thread.join(); }
        VERIFY_ARE_EQUAL(size_t{ 0 }, failures.load());
        VERIFY_ARE_EQUAL(size_t{ 1 }, winners.load());
        VERIFY_ARE_EQUAL(size_t{ 1 }, store.Get().leases.size());
        VERIFY_IS_FALSE(store.Release(destination.id, firstOwner, first.nonce));
    }

    void PinnedStoreTests::WindowExitAndSurvivorAccess()
    {
        P::Store store{ root(), alive }; const auto a = owner(1), b = owner(2), d = owner(4);
        const auto one = pin(), two = pin(); store.Add(one, a); store.Add(two, b);
        store.ReleaseWindow(b);
        const auto unviewed = store.ClaimUnviewed(d);
        VERIFY_ARE_EQUAL(size_t{ 1 }, unviewed.size()); VERIFY_ARE_EQUAL(two.id, unviewed[0].first.id);
        VERIFY_ARE_EQUAL(size_t{ 2 }, store.Get().catalog.pins.size());
        store.ReleaseWindow(a); store.ReleaseWindow(d);
        VERIFY_ARE_EQUAL(size_t{ 2 }, store.ClaimUnviewed(owner(5)).size());
        VERIFY_ARE_EQUAL(size_t{ 2 }, store.Get().catalog.pins.size());
    }

    void PinnedStoreTests::CrashAndPidReuseRecovery()
    {
        auto life = P::Life::Alive;
        P::Store store{ root(), [&](const auto&) { return life; } };
        const auto a = owner(1); const auto destination = pin(); store.Add(destination, a);
        life = P::Life::WindowGone;
        VERIFY_IS_TRUE(store.Get().leases.empty()); VERIFY_ARE_EQUAL(size_t{ 1 }, store.Get().catalog.pins.size());
        life = P::Life::Alive; const auto claim = store.Claim(destination.id, a);
        store.BeginTransfer(destination.id, a, claim->nonce, 2);
        life = P::Life::ProcessGone;
        VERIFY_IS_TRUE(store.Get().leases.empty()); VERIFY_ARE_EQUAL(size_t{ 1 }, store.Get().catalog.pins.size());
        auto reused = owner(1); reused.process = GetCurrentProcessId(); reused.started = 1;
        VERIFY_IS_TRUE(P::Probe(reused) == P::Life::ProcessGone);
    }

    void PinnedStoreTests::TransferFenceAndStaleActions()
    {
        auto life = P::Life::Alive;
        P::Store store{ root(), [&](const auto&) { return life; } };
        const auto a = owner(1), b = owner(2), c = owner(3); const auto destination = pin();
        const auto original = store.Add(destination, a);
        const auto transfer = store.BeginTransfer(destination.id, a, original.nonce, b.windowId);
        VERIFY_IS_FALSE(store.ReleaseWindow(a));
        life = P::Life::WindowGone;
        VERIFY_IS_FALSE(store.Claim(destination.id, c).has_value());
        VERIFY_ARE_EQUAL(size_t{ 1 }, store.Get().leases.size());
        life = P::Life::Alive;
        const auto moved = store.AcceptTransfer(destination.id, transfer, b);
        VERIFY_IS_TRUE(moved.owner == b); VERIFY_IS_TRUE(moved.nonce != original.nonce);
        VERIFY_IS_FALSE(store.Release(destination.id, a, original.nonce));
        VERIFY_THROWS(store.Remove(destination.id, a, original.nonce), std::exception);
        VERIFY_ARE_EQUAL(destination.path, store.Get().catalog.pins[0].path);
        VERIFY_IS_FALSE(store.ReleaseWindow(b));
        VERIFY_IS_FALSE(store.Claim(destination.id, c).has_value());
        VERIFY_THROWS(store.Remove(destination.id, b, moved.nonce), std::exception);
        VERIFY_IS_TRUE(store.CompleteTransfer(destination.id, transfer, b, moved.nonce));
        VERIFY_IS_TRUE(store.Remove(destination.id, b, moved.nonce));
        VERIFY_IS_TRUE(store.Get().catalog.pins.empty()); VERIFY_IS_TRUE(store.Get().leases.empty());
    }

    void PinnedStoreTests::FailedTransferRecovery()
    {
        P::Store store{ root(), alive }; const auto a = owner(1); const auto destination = pin();
        auto lease = store.Add(destination, a);
        const auto first = store.BeginTransfer(destination.id, a, lease.nonce, 2);
        VERIFY_IS_TRUE(store.FinishFailedTransfer(destination.id, first, a));
        lease = store.Get().leases.at(destination.id);
        VERIFY_IS_TRUE(lease.owner == a); VERIFY_IS_TRUE(lease.transfer.empty());
        VERIFY_IS_FALSE(store.FinishFailedTransfer(destination.id, first, std::nullopt));
        const auto second = store.BeginTransfer(destination.id, a, lease.nonce, 2);
        VERIFY_IS_TRUE(store.FinishFailedTransfer(destination.id, second, std::nullopt));
        VERIFY_IS_TRUE(store.Get().leases.empty()); VERIFY_ARE_EQUAL(size_t{ 1 }, store.Get().catalog.pins.size());
        VERIFY_IS_TRUE(store.Claim(destination.id, owner(3)).has_value());
    }

    void PinnedStoreTests::WriteFailureLeavesDiskUnchanged()
    {
        const auto directory = root(); P::Store good{ directory, alive };
        const auto a = owner(1); const auto destination = pin(); const auto lease = good.Add(destination, a);
        P::Store broken{ directory, alive, [](const auto&, auto) { throw std::runtime_error("Injected atomic write failure"); } };
        const auto before = read(directory / "pins.json");
        VERIFY_THROWS(broken.Remove(destination.id, a, lease.nonce), std::exception);
        VERIFY_THROWS(broken.Add(pin(), owner(2)), std::exception);
        VERIFY_ARE_EQUAL(before, read(directory / "pins.json"));
        good.Release(destination.id, a, lease.nonce);
        const auto released = read(directory / "pins.json");
        VERIFY_THROWS(broken.Claim(destination.id, a), std::exception);
        VERIFY_ARE_EQUAL(released, read(directory / "pins.json"));
        VERIFY_IS_TRUE(good.Get().leases.empty());
    }

    void PinnedStoreTests::SettingsAndCopiesRemainIndependent()
    {
        const auto directory = root(); const auto other = root();
        write(directory / "settings.json", "fixture pending settings draft");
        write(directory / "state.json", "fixture seen GUIDs and window layouts");
        P::Store store{ directory, alive }; const auto destination = pin(); const auto a = owner(1);
        const auto lease = store.Add(destination, a); store.Remove(destination.id, a, lease.nonce);
        VERIFY_ARE_EQUAL(std::string{ "fixture pending settings draft" }, read(directory / "settings.json"));
        VERIFY_ARE_EQUAL(std::string{ "fixture seen GUIDs and window layouts" }, read(directory / "state.json"));
        store.Add(pin(), a);
        VERIFY_IS_TRUE(P::Store(other, alive).Get().catalog.pins.empty());
    }

    void PinnedStoreTests::InvalidDataIsNotOverwritten()
    {
        const auto directory = root(); P::Store store{ directory, alive };
        for (const auto* invalid : { "", "not json", "{\"version\":2,\"catalog\":{},\"leases\":[]}", "{\"version\":1,\"version\":1}" })
        {
            write(directory / "pins.json", invalid);
            VERIFY_THROWS(store.Get(), std::exception);
            VERIFY_THROWS(store.Add(pin(), owner(1)), std::exception);
            VERIFY_ARE_EQUAL(std::string{ invalid }, read(directory / "pins.json"));
        }
    }
    void PinnedStoreTests::ExpiredRecoveryKeepsFenceUntilAttached()
    {
        const auto path = root(); P::Store store{ path, alive }; const auto a = owner(1), b = owner(2); const auto destination = pin();
        const auto original = store.Add(destination, a); const auto ticket = store.BeginTransfer(destination.id, a, original.nonce, b.windowId);
        const auto file = path / L"pins.json"; const auto content = read(file); Json::CharReaderBuilder builder; std::unique_ptr<Json::CharReader> parser{ builder.newCharReader() }; Json::Value value; std::string errors;
        VERIFY_IS_TRUE(parser->parse(content.data(), content.data() + content.size(), &value, &errors)); value["leases"][0]["transferUntil"] = Json::UInt64{ 1 }; write(file, Json::writeString(Json::StreamWriterBuilder{}, value));
        VERIFY_THROWS(store.AcceptTransfer(destination.id, ticket, b), std::exception);
        VERIFY_THROWS(store.RecoverTransfer(destination.id, ticket, b), std::exception);
        const auto restored = store.RecoverTransfer(destination.id, ticket, a);
        VERIFY_IS_TRUE(restored.nonce != original.nonce); VERIFY_IS_TRUE(restored.transfer == ticket);
        VERIFY_IS_FALSE(store.Claim(destination.id, b).has_value());
        VERIFY_THROWS(store.CompleteTransfer(destination.id, ticket, a, original.nonce), std::exception);
        VERIFY_IS_TRUE(store.CompleteTransfer(destination.id, ticket, a, restored.nonce));
        VERIFY_IS_TRUE(store.Release(destination.id, a, restored.nonce)); VERIFY_IS_TRUE(store.Claim(destination.id, b).has_value());
        VERIFY_ARE_EQUAL(size_t{ 1 }, store.Get().catalog.pins.size());
    }
    void PinnedStoreTests::AcceptedCompletionWriteFailure()
    {
        const auto path = root(); bool fail = false;
        P::Store store{ path, alive, [&](const auto& file, std::string_view content) { if (fail) { throw std::runtime_error("Injected completion failure"); } til::io::write_utf8_string_to_file_atomic(file, content); } };
        const auto a = owner(1), b = owner(2), c = owner(3); const auto destination = pin(); const auto original = store.Add(destination, a);
        const auto ticket = store.BeginTransfer(destination.id, a, original.nonce, b.windowId); const auto moved = store.AcceptTransfer(destination.id, ticket, b);
        const auto before = read(path / L"pins.json"); fail = true;
        VERIFY_THROWS(store.CompleteTransfer(destination.id, ticket, b, moved.nonce), std::exception);
        VERIFY_ARE_EQUAL(before, read(path / L"pins.json")); VERIFY_IS_FALSE(store.Claim(destination.id, c).has_value()); VERIFY_IS_FALSE(store.ReleaseWindow(b));
        fail = false; VERIFY_IS_TRUE(store.CompleteTransfer(destination.id, ticket, b, moved.nonce)); VERIFY_IS_TRUE(store.ReleaseWindow(b));
        VERIFY_IS_TRUE(store.Claim(destination.id, c).has_value()); VERIFY_ARE_EQUAL(size_t{ 1 }, store.Get().catalog.pins.size());
    }
    void PinnedStoreTests::ClosedRemovalRejectsNewClaim()
    {
        P::Store store{ root(), alive }; const auto a = owner(1); const auto first = pin(), second = pin();
        const auto lease = store.Add(first, a); store.Add(second, a);
        VERIFY_THROWS(store.RemoveUnviewed(first.id), std::exception);
        VERIFY_ARE_EQUAL(size_t{ 2 }, store.Get().catalog.pins.size());
        VERIFY_IS_TRUE(store.Release(first.id, a, lease.nonce)); VERIFY_IS_TRUE(store.RemoveUnviewed(first.id)); VERIFY_IS_FALSE(store.RemoveUnviewed(first.id));
        const auto state = store.Get(); VERIFY_ARE_EQUAL(size_t{ 1 }, state.catalog.pins.size()); VERIFY_ARE_EQUAL(second.id, state.catalog.pins[0].id); VERIFY_ARE_EQUAL(size_t{ 1 }, state.leases.size());
    }

}
