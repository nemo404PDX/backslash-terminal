// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <cstdint>
#include <winrt/Windows.Data.Xml.Dom.h>
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

    struct BezelBadgePaint
    {
        uint32_t rgb;
        bool none = false;
    };

    // Only normalized RGB or None reaches runtime SVG attributes.
    inline BezelBadgePaint ParseBezelBadgePaint(std::wstring_view value, uint32_t fallback) noexcept
    {
        if (value == L"none") { return { fallback, true }; }
        if (value.size() != 7 || value.front() != L'#') { return { fallback }; }
        uint32_t rgb = 0;
        for (const auto ch : value.substr(1))
        {
            const auto digit = ch >= L'0' && ch <= L'9' ? ch - L'0' :
                               ch >= L'a' && ch <= L'f' ? ch - L'a' + 10 :
                               ch >= L'A' && ch <= L'F' ? ch - L'A' + 10 : -1;
            if (digit < 0) { return { fallback }; }
            rgb = (rgb << 4) | static_cast<uint32_t>(digit);
        }
        return { rgb };
    }

    inline std::wstring BezelBadgePaintText(BezelBadgePaint paint)
    {
        if (paint.none) { return L"none"; }
        std::wstring result{ L"#000000" };
        constexpr auto hex = L"0123456789ABCDEF";
        for (size_t i = 0; i < 6; ++i) { result[6 - i] = hex[(paint.rgb >> (i * 4)) & 15]; }
        return result;
    }

    inline std::string BadgeSvgWithIndependentPaint(const std::string& svg, std::wstring_view lettering, std::wstring_view backing)
    {
        using namespace winrt::Windows::Data::Xml::Dom;
        XmlDocument document;
        XmlLoadSettings settings;
        settings.ProhibitDtd(true);
        settings.ResolveExternals(false);
        document.LoadXml(winrt::to_hstring(svg), settings);
        const std::array layers{
            std::pair{ L"letters", ParseBezelBadgePaint(lettering, 0xffffff) },
            std::pair{ L"backer", ParseBezelBadgePaint(backing, 0x000000) }
        };
        for (const auto& [id, paint] : layers)
        {
            const auto query = std::wstring{ L"//*[@id='" } + id + L"']";
            const auto layer = document.SelectSingleNode(query).try_as<XmlElement>();
            THROW_HR_IF(E_INVALIDARG, !layer);
            const auto paths = layer.SelectNodes(L".//*[local-name()='path']");
            THROW_HR_IF(E_INVALIDARG, paths.Length() == 0);
            const auto fill = BezelBadgePaintText(paint);
            for (const auto& node : paths)
            {
                const auto path = node.as<XmlElement>();
                // Override only paint in a derived in-memory document. A local
                // inline fill also overrides the author's currentColor style.
                const auto style = std::wstring{ path.GetAttribute(L"style") } + L";fill:" + fill + L";";
                path.SetAttribute(L"style", style);
            }
        }
        return winrt::to_string(document.GetXml());
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
            return result;
        }();
        return assets;
    }

    inline winrt::fire_and_forget LoadBezelBadgeSvg(winrt::Windows::UI::Xaml::Media::Imaging::SvgImageSource source, int style, std::wstring lettering = L"#FFFFFF", std::wstring backing = L"#000000")
    {
        try
        {
            // Percentage SVG dimensions have no finite intrinsic raster viewport.
            // XAML otherwise paints a small square even after a successful
            // decode. Match the artwork viewBox; Image still lays out in DIPs.
            source.RasterizePixelWidth(527);
            source.RasterizePixelHeight(100);
            using namespace winrt::Windows::Storage::Streams;
            const auto& asset = BezelBadgeAssets()[style >= 0 && style < 3 ? style : 0];
            const auto svg = style == 2 ? BadgeSvgWithIndependentPaint(asset, lettering, backing) : asset;
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
