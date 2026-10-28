// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.Terminal.Settings.Model.h>
#include <cstdio>
#include <exception>
#include <string>
#include <vector>

namespace Microsoft::Terminal::Backslash::SshScanUi
{
    // A typed IVectorView<Profile> is valid for the model API, but a plain
    // single_threaded_vector does not expose IIterable<IInspectable> to XAML.
    // Keep actual Profile objects (and their GUIDs), in an ItemsSource shape
    // accepted by ListView. This adapter never changes the Settings draft.
    inline auto Rows(const winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Terminal::Settings::Model::Profile>& candidates)
    {
        std::vector<winrt::Windows::Foundation::IInspectable> rows;
        rows.reserve(candidates.Size());
        for (const auto& candidate : candidates) { rows.emplace_back(candidate); }
        return winrt::single_threaded_observable_vector(std::move(rows));
    }

    inline winrt::hstring Failure(std::wstring_view stage, const winrt::hresult_error& error)
    {
        wchar_t code[11]{};
        swprintf_s(code, L"0x%08X", static_cast<unsigned int>(error.code().value));
        std::wstring message{L"Couldn't scan SSH connections. No profiles were added. Try again or add a profile manually.\nDetails: "};
        message.append(stage).append(L" (").append(code).append(L"): ").append(error.message());
        return winrt::hstring{message};
    }

    inline winrt::hstring Failure(std::wstring_view stage, const std::exception& error)
    {
        std::wstring message{L"Couldn't scan SSH connections. No profiles were added. Try again or add a profile manually.\nDetails: "};
        message.append(stage).append(L": ").append(winrt::to_hstring(error.what()));
        return winrt::hstring{message};
    }
}
