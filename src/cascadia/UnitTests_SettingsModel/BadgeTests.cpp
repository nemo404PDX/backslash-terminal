// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "pch.h"
#include "JsonTestClass.h"
#include "../TerminalSettingsModel/Profile.h"
#include "../TerminalSettingsModel/EnumMappings.h"
#include "../inc/BezelBadgeLayout.h"
#include "../inc/BezelBadgeSvg.h"
#include <limits>

using namespace winrt;
using namespace winrt::Microsoft::Terminal::Settings::Model;
using namespace WEX::TestExecution;
namespace B = ::Microsoft::Terminal::Backslash;
namespace M = winrt::Microsoft::Terminal::Settings::Model::implementation;

namespace SettingsModelUnitTests
{
    class BadgeTests : public JsonTestClass
    {
        TEST_CLASS(BadgeTests);
        TEST_METHOD(SettingsAndLegacyRoundtrip);
        TEST_METHOD(PlacementSizingAndDpi);
        TEST_METHOD(BrandingAndFillerAreIndependent);
        TEST_METHOD(AssetColorAndIdentity);
    };

    void BadgeTests::SettingsAndLegacyRoundtrip()
    {
        const auto defaults = make_self<M::Profile>();
        VERIFY_ARE_EQUAL(RetroBezelBadge::None, defaults->RetroBezelBadge());
        VERIFY_ARE_EQUAL(RetroBezelFiller::None, defaults->RetroBezelFiller());
        VERIFY_ARE_EQUAL(RetroBezelBadgeStyle::Solid, defaults->RetroBezelBadgeStyle());
        VERIFY_ARE_EQUAL(7u, EnumMappings::RetroBezelBadge().Size());
        // The UI order is independent of the historical serialized identities.
        const std::array<std::pair<std::string_view, int>, 7> savedPositions{
            std::pair{ "none", 0 }, { "topLeft", 5 }, { "topCenter", 2 }, { "topRight", 6 },
            { "bottomLeft", 3 }, { "bottomCenter", 1 }, { "bottomRight", 4 }
        };
        for (const auto& [token, value] : savedPositions)
        {
            Json::Value json;
            json["experimental.retroBezelBadge"] = std::string{ token };
            const auto profile = M::Profile::FromJson(json);
            VERIFY_ARE_EQUAL(value, static_cast<int>(profile->RetroBezelBadge()));
            VERIFY_ARE_EQUAL(profile->RetroBezelBadge(), EnumMappings::RetroBezelBadge().Lookup(to_hstring(token)));
            VERIFY_ARE_EQUAL(std::string{ token }, profile->ToJson()["experimental.retroBezelBadge"].asString());
        }
        for (const auto token : { "none", "topLeft", "topCenter", "topRight", "bottomLeft", "bottomCenter", "bottomRight" })
        {
            for (const auto style : { "solid", "outline", "currentColor" })
            {
                Json::Value json;
                json["experimental.retroBezelBadge"] = token;
                json["experimental.retroBezelBadgeStyle"] = style;
                json["experimental.retroBezelFiller"] = "top";
                const auto profile = M::Profile::FromJson(json);
                const auto saved = profile->ToJson();
                VERIFY_ARE_EQUAL(std::string{ token }, saved["experimental.retroBezelBadge"].asString());
                VERIFY_ARE_EQUAL(std::string{ style }, saved["experimental.retroBezelBadgeStyle"].asString());
                const auto loaded = M::Profile::FromJson(saved);
                VERIFY_ARE_EQUAL(profile->RetroBezelBadge(), loaded->RetroBezelBadge());
                VERIFY_ARE_EQUAL(profile->RetroBezelBadgeStyle(), loaded->RetroBezelBadgeStyle());
                VERIFY_ARE_EQUAL(RetroBezelFiller::Top, loaded->RetroBezelFiller());
                const auto copy = profile->CopySettings();
                VERIFY_ARE_EQUAL(profile->RetroBezelBadgeStyle(), copy->RetroBezelBadgeStyle());
                VERIFY_ARE_EQUAL(RetroBezelFiller::Top, copy->RetroBezelFiller());
            }
        }
        for (const auto token : { "top", "bottom" })
        {
            Json::Value json;
            json["experimental.retroBezelBadge"] = token;
            const auto profile = M::Profile::FromJson(json);
            const auto expected = std::string{ token } == "top" ? RetroBezelFiller::Top : RetroBezelFiller::Bottom;
            VERIFY_ARE_EQUAL(expected, profile->RetroBezelFiller());
            VERIFY_ARE_EQUAL(std::string{ token } + "Center", profile->ToJson()["experimental.retroBezelBadge"].asString());
            profile->RetroBezelBadge(RetroBezelBadge::None);
            VERIFY_ARE_EQUAL(expected, profile->RetroBezelFiller());
            VERIFY_ARE_EQUAL(expected, M::Profile::FromJson(profile->ToJson())->RetroBezelFiller());
            json["experimental.retroBezelFiller"] = "none";
            VERIFY_ARE_EQUAL(RetroBezelFiller::None, M::Profile::FromJson(json)->RetroBezelFiller());
        }
        const auto child = make_self<M::Profile>();
        defaults->RetroBezelFiller(RetroBezelFiller::Bottom);
        defaults->RetroBezelBadgeStyle(RetroBezelBadgeStyle::Outline);
        child->AddLeastImportantParent(defaults);
        VERIFY_ARE_EQUAL(RetroBezelFiller::Bottom, child->RetroBezelFiller());
        child->RetroBezelFiller(RetroBezelFiller::None);
        child->RetroBezelBadgeStyle(RetroBezelBadgeStyle::CurrentColor);
        child->LayerJson(VerifyParseSucceeded(R"({"experimental.retroBezelFiller":null,"experimental.retroBezelBadgeStyle":null})"));
        VERIFY_ARE_EQUAL(RetroBezelFiller::Bottom, child->RetroBezelFiller());
        VERIFY_ARE_EQUAL(RetroBezelBadgeStyle::Outline, child->RetroBezelBadgeStyle());
    }

    void BadgeTests::PlacementSizingAndDpi()
    {
        for (int size = 0; size < 3; ++size)
        for (int filler = 0; filler < 3; ++filler)
        for (int placement = 1; placement <= 6; ++placement)
        for (const auto width : { 20.f, 60.f, 200.f, 240.f, 320.f, 640.f, 1800.f })
        for (const auto height : { 100.f, 600.f })
        {
            const auto insets = B::GetBezelInsets(true, size, placement, false, filler);
            const auto layout = B::GetBezelBadgeLayout(true, placement, width, height, insets);
            const auto opening = B::GetBezelApertureBounds(width, height, insets).opening;
            const auto allowedLeft = opening.left + B::BezelBadgeCornerInset;
            const auto allowedRight = opening.right - B::BezelBadgeCornerInset;
            if (allowedRight <= allowedLeft) { VERIFY_IS_FALSE(layout.visible); continue; }
            VERIFY_IS_TRUE(layout.visible);
            VERIFY_IS_TRUE(layout.left >= allowedLeft - 0.001f);
            VERIFY_IS_TRUE(layout.left + layout.width <= allowedRight + 0.001f);
            VERIFY_IS_TRUE(layout.left >= insets.left && layout.left + layout.width <= width - insets.right);
            if (placement == 3 || placement == 5) { VERIFY_IS_TRUE(std::abs(layout.left - allowedLeft) < 0.001f); }
            else if (placement == 4 || placement == 6) { VERIFY_IS_TRUE(std::abs(layout.left + layout.width - allowedRight) < 0.001f); }
            else { VERIFY_IS_TRUE(std::abs(layout.left + layout.width / 2 - width / 2) < 0.001f); }
            VERIFY_IS_TRUE(layout.top >= 0 && layout.top + layout.height <= height);
            VERIFY_IS_TRUE(std::abs(layout.width / layout.height - B::BezelBadgeAspectRatio) < 0.001f);
            VERIFY_IS_TRUE(layout.height <= B::BezelBadgeHeight);
            const bool top = placement == 2 || placement == 5 || placement == 6;
            const auto shoulder = (top ? insets.top : insets.bottom) - B::BezelInnerClearance - B::BezelRecessThickness;
            const auto center = B::BezelOuterRimThickness + (std::min(shoulder, height) - B::BezelOuterRimThickness) / 2;
            VERIFY_IS_TRUE(std::abs((top ? layout.top + layout.height / 2 : height - layout.top - layout.height / 2) - center) < 0.001f);
            for (const auto scale : { 1.f, 1.25f, 1.5f, 2.f })
            {
                // Layout takes DIPs; XAML scales the same un-clipped rectangle.
                const auto scaled = B::GetBezelBadgeLayout(true, placement, width * scale / scale, height * scale / scale, insets);
                VERIFY_IS_TRUE(std::abs(scaled.width - layout.width) < 0.001f);
                VERIFY_IS_TRUE(scaled.left * scale + scaled.width * scale <= allowedRight * scale + 0.01f);
            }
        }
        VERIFY_IS_FALSE(B::GetBezelBadgeLayout(true, 1, std::numeric_limits<float>::quiet_NaN(), 600, {}).visible);
        VERIFY_IS_FALSE(B::GetBezelBadgeLayout(true, 1, 640, 0, {}).visible);
    }

    void BadgeTests::BrandingAndFillerAreIndependent()
    {
        for (int filler = 0; filler < 3; ++filler)
        for (int placement = 0; placement <= 6; ++placement)
        {
            const auto insets = B::GetBezelInsets(true, 1, placement, false, filler);
            VERIFY_ARE_EQUAL(160.f, insets.Width());
            VERIFY_ARE_EQUAL(filler == 0 ? 160.f : 188.f, insets.Height());
            const auto balanced = B::GetBezelInsetsForMode(true, 1, placement, true, true, filler);
            VERIFY_ARE_EQUAL(filler == 0 ? 160.f : 216.f, balanced.Height());
            const auto disabled = B::GetBezelInsets(false, 1, placement, true, filler);
            VERIFY_ARE_EQUAL(0.f, disabled.Height());
            VERIFY_IS_FALSE(B::GetBezelBadgeLayout(false, placement, 640, 600, insets).visible);
            if (placement == 0) { VERIFY_IS_FALSE(B::GetBezelBadgeLayout(true, placement, 640, 600, insets).visible); }
        }
    }

    void BadgeTests::AssetColorAndIdentity()
    {
        VERIFY_ARE_EQUAL(3u, static_cast<unsigned int>(B::BezelBadgeAssetNames.size()));
        VERIFY_ARE_EQUAL(std::wstring_view{ L"backslash-badge.svg" }, B::BezelBadgeAssetNames[0]);
        const std::string svg = R"(<svg viewBox="0 0 527 100"><g id="backer"><path fill="#000" d="M0 0"/></g><g id="letters" fill="currentColor"><path d="M1 1"/></g></svg>)";
        const auto colored = B::BadgeSvgWithWhiteCurrentColor(svg);
        VERIFY_ARE_EQUAL(svg.size() + 16u, colored.size());
        VERIFY_IS_TRUE(colored.find("<svg color=\"#FFFFFF\"") == 0);
        VERIFY_ARE_EQUAL(svg.substr(4), colored.substr(20));
    }
}
