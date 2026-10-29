// Copyright (c) Backslash contributors. Licensed under the MIT license.
#pragma once
#include <string>
#include <stdexcept>
namespace Backslash::ModernExplorer::Management
{
    enum class Action { Enable, Repair, Remove };
    struct State
    {
        bool supported{}, safe{ true }, developerMode{}, bundle{}, package{}, config{}, supportExists{}, filesComplete{}, configComplete{}, targetValid{};
        std::wstring current, target, support, details, problem, legacy;
    };
    inline bool Same(const std::wstring& a, const std::wstring& b) { return _wcsicmp(a.c_str(), b.c_str()) == 0; }
    struct Plan { bool files{}, registration{}, target{}, removePackage{}, removeTarget{}, removeFiles{}; std::wstring selected; bool Changes() const { return files || registration || target || removePackage || removeTarget || removeFiles; } };
    inline Plan MakePlan(Action action, const State& state, const std::wstring& expected)
    {
        if (!state.supported || !state.safe) { throw std::runtime_error("This integration state cannot be changed safely. See the visible status and Details."); }
        if (!Same(state.target, expected)) { throw std::runtime_error("The selected copy changed. Check status and confirm again."); }
        Plan plan;
        if (action == Action::Remove) { plan.removePackage = state.package; plan.removeTarget = state.config; plan.removeFiles = state.supportExists; return plan; }
        plan.selected = action == Action::Repair && !state.target.empty() ? state.target : state.current;
        if (action == Action::Repair && !state.target.empty() && !state.targetValid) { throw std::runtime_error("The selected portable copy is unavailable. Use this copy explicitly to choose a replacement."); }
        plan.files = !state.filesComplete;
        plan.registration = !state.package;
        plan.target = !state.configComplete || !Same(plan.selected, state.target);
        if (plan.files && !state.bundle) { throw std::runtime_error("This portable folder is missing the reviewed Explorer helper payload. Restore the complete build folder."); }
        if (plan.registration && !state.developerMode) { throw std::runtime_error("This development helper requires Developer Mode already enabled. No policy or certificate changes were made. Signed distribution is not available in this build."); }
        return plan;
    }
    struct Backend
    {
        virtual ~Backend() = default;
        virtual State Read() = 0;
        virtual void Begin(const State&) = 0;
        virtual void Validate(const std::wstring&) = 0;
        virtual void InstallFiles() = 0;
        virtual void Register() = 0;
        virtual void Select(const std::wstring&, const std::wstring&) = 0;
        virtual void RemovePackage() = 0;
        virtual void RemoveTarget(const std::wstring&) = 0;
        virtual void RemoveFiles() = 0;
        virtual void Rollback() = 0;
        virtual bool Changed() const = 0;
        virtual void Notify() noexcept = 0;
        virtual std::wstring FailureMessage() noexcept = 0;
    };
    struct Result { bool success{}, changed{}; std::wstring error; };
    inline Result Execute(Backend& backend, Action action, const std::wstring& expected)
    {
        Result result;
        bool began = false;
        try
        {
            const auto state = backend.Read(); const auto plan = MakePlan(action, state, expected);
            if (!plan.Changes()) { result.success = true; return result; }
            if (action != Action::Remove) { backend.Validate(plan.selected); }
            backend.Begin(state); began = true;
            if (plan.files) { backend.InstallFiles(); }
            if (plan.registration) { backend.Register(); }
            if (plan.target) { backend.Select(plan.selected, expected); }
            if (plan.removePackage) { backend.RemovePackage(); }
            if (plan.removeTarget) { backend.RemoveTarget(expected); }
            if (plan.removeFiles) { backend.RemoveFiles(); }
            result.success = true;
        }
        catch (...)
        {
            result.error = backend.FailureMessage();
            if (began && action != Action::Remove)
            {
                try { backend.Rollback(); }
                catch (...) { result.error += L"\nRollback stopped to preserve changed or foreign state: " + backend.FailureMessage(); }
            }
        }
        result.changed = backend.Changed();
        if (result.changed) { backend.Notify(); }
        return result;
    }
}
