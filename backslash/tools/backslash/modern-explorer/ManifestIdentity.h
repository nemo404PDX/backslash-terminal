// Copyright (c) Backslash contributors. Licensed under the MIT license.
#pragma once
#include "TargetStore.h"
#include <appxpackaging.h>
#include <shlwapi.h>
#include <wrl.h>
namespace Backslash::ModernExplorer
{
    inline void CheckHR(HRESULT code, const char* phase) { if (FAILED(code)) { throw Error(code, phase); } }
    inline std::wstring IdentityJson(const std::wstring& manifest)
    {
        CheckHR(CoInitializeEx(nullptr, COINIT_MULTITHREADED), "Initialize manifest reader");
        struct Cleanup { ~Cleanup() { CoUninitialize(); } } cleanup;
        Microsoft::WRL::ComPtr<IStream> stream;
        CheckHR(SHCreateStreamOnFileEx(manifest.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE, 0, FALSE, nullptr, &stream), "Read manifest");
        Microsoft::WRL::ComPtr<IAppxFactory> factory;
        CheckHR(CoCreateInstance(__uuidof(AppxFactory), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)), "Create manifest reader");
        Microsoft::WRL::ComPtr<IAppxManifestReader> reader; CheckHR(factory->CreateManifestReader(stream.Get(), &reader), "Parse package manifest");
        Microsoft::WRL::ComPtr<IAppxManifestPackageId> id; CheckHR(reader->GetPackageId(&id), "Read package identity");
        const auto get = [&](auto method) { LPWSTR value{}; CheckHR((id.Get()->*method)(&value), "Read identity property"); std::wstring text(value); CoTaskMemFree(value); return text; };
        return L"{\"name\":" + Json(get(&IAppxManifestPackageId::GetName)) + L",\"publisher\":" + Json(get(&IAppxManifestPackageId::GetPublisher)) + L",\"family\":" + Json(get(&IAppxManifestPackageId::GetPackageFamilyName)) + L",\"fullName\":" + Json(get(&IAppxManifestPackageId::GetPackageFullName)) + L"}";
    }
}
