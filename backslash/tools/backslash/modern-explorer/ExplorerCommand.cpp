// Copyright (c) Backslash contributors. Licensed under the MIT license.
#include "ExplorerCommand.h"
using namespace Backslash::ModernExplorer;
struct Factory final : Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IClassFactory>
{
    Factory() { ++Objects; } ~Factory() { --Objects; }
    IFACEMETHODIMP CreateInstance(IUnknown* outer, REFIID id, void** object) noexcept override
    {
        if (!object) { return E_POINTER; } *object = nullptr; if (outer) { return CLASS_E_NOAGGREGATION; }
        return Guard([&]() -> HRESULT { const auto command = Microsoft::WRL::Make<Command>(); return command ? command.CopyTo(id, object) : E_OUTOFMEMORY; });
    }
    IFACEMETHODIMP LockServer(BOOL lock) noexcept override { if (lock) { ++Locks; } else { --Locks; } return S_OK; }
};
STDAPI DllCanUnloadNow() { return Objects == 0 && Locks == 0 ? S_OK : S_FALSE; }
STDAPI DllGetClassObject(REFCLSID clsid, REFIID id, void** object)
{
    if (!object) { return E_POINTER; } *object = nullptr; if (clsid != __uuidof(Command)) { return CLASS_E_CLASSNOTAVAILABLE; }
    return Guard([&]() -> HRESULT { const auto factory = Microsoft::WRL::Make<Factory>(); return factory ? factory.CopyTo(id, object) : E_OUTOFMEMORY; });
}
