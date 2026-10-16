// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.Xaml.Media.Imaging.h>

namespace Microsoft::Terminal::Backslash
{
    inline constexpr std::array<std::wstring_view, 3> BezelBadgeAssetNames{
        L"backslash-badge.svg", L"backslash-badge-outline.svg", L"backslash-badge-currentcolor.svg"
    };

    inline std::string BadgeSvgWithWhiteCurrentColor(std::string svg)
    {
        // The supplied path/group fills stay intact. Set SVG color explicitly;
        // XAML Foreground cannot inherit through an external image boundary.
        if (const auto root = svg.find("<svg"); root != std::string::npos)
        {
            svg.insert(root + 4, " color=\"#FFFFFF\"");
        }
        return svg;
    }

    inline const std::array<std::string, 3>& BezelBadgeAssets()
    {
        static const auto assets = [] {
            std::array<std::string, 3> result;
            const auto directory = std::filesystem::path{ wil::GetModuleFileNameW<std::wstring>(nullptr) }.parent_path() / L"Assets" / L"branding";
            for (size_t i = 0; i < result.size(); ++i)
            {
                std::ifstream file{ directory / BezelBadgeAssetNames[i], std::ios::binary };
                THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), !file);
                result[i].assign(std::istreambuf_iterator<char>{ file }, {});
                THROW_HR_IF(E_INVALIDARG, result[i].empty() || result[i].size() > 65536);
            }
            result[2] = BadgeSvgWithWhiteCurrentColor(std::move(result[2]));
            return result;
        }();
        return assets;
    }

    inline winrt::fire_and_forget LoadBezelBadgeSvg(winrt::Windows::UI::Xaml::Media::Imaging::SvgImageSource source, int style)
    {
        try
        {
            // Percentage SVG dimensions have no finite intrinsic raster viewport.
            // XAML otherwise paints a small square even after a successful
            // decode. Match the artwork viewBox; Image still lays out in DIPs.
            source.RasterizePixelWidth(527);
            source.RasterizePixelHeight(100);
            using namespace winrt::Windows::Storage::Streams;
            const auto& svg = BezelBadgeAssets()[style >= 0 && style < 3 ? style : 0];
            InMemoryRandomAccessStream stream;
            DataWriter writer{ stream };
            writer.WriteBytes({ reinterpret_cast<const uint8_t*>(svg.data()), static_cast<uint32_t>(svg.size()) });
            co_await writer.StoreAsync();
            writer.DetachStream();
            stream.Seek(0);
            const auto status = co_await source.SetSourceAsync(stream);
            LOG_HR_IF(E_FAIL, status != winrt::Windows::UI::Xaml::Media::Imaging::SvgImageSourceLoadStatus::Success);
        }
        CATCH_LOG();
        // Own only the source. A stale load cannot reattach to a newer Image or
        // dereference a destroyed RetroBezel during rapid changes/navigation.
    }
}
