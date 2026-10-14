/*++
Copyright (c) Microsoft Corporation
Licensed under the MIT license.

Module Name:
- AppearanceConfig

Abstract:
- The implementation of the AppearanceConfig winrt class. Provides settings related
  to the appearance of the terminal, in both terminal control and terminal core.

Author(s):
- Pankaj Bhojwani - Nov 2020

--*/

#pragma once

#include "AppearanceConfig.g.h"
#include "JsonUtils.h"
#include "IInheritable.h"
#include "MTSMSettings.h"
#include "MediaResourceSupport.h"
#include <DefaultSettings.h>

namespace winrt::Microsoft::Terminal::Settings::Model::implementation
{
    struct AppearanceConfig : AppearanceConfigT<AppearanceConfig, IMediaResourceContainer>, IInheritable<AppearanceConfig>
    {
    public:
        AppearanceConfig(winrt::weak_ref<Profile> sourceProfile);
        static winrt::com_ptr<AppearanceConfig> CopyAppearance(const AppearanceConfig* source, winrt::weak_ref<Profile> sourceProfile);
        Json::Value ToJson() const;
        void LayerJson(const Json::Value& json);
        void LogSettingChanges(std::set<std::string>& changes, const std::string_view& context) const;

        Model::Profile SourceProfile();

        void ResolveMediaResources(const Model::MediaResourceResolver& resolver);

        INHERITABLE_NULLABLE_SETTING(Model::IAppearanceConfig, Microsoft::Terminal::Core::Color, Foreground, nullptr);
        INHERITABLE_NULLABLE_SETTING(Model::IAppearanceConfig, Microsoft::Terminal::Core::Color, Background, nullptr);
        INHERITABLE_NULLABLE_SETTING(Model::IAppearanceConfig, Microsoft::Terminal::Core::Color, SelectionBackground, nullptr);
        INHERITABLE_NULLABLE_SETTING(Model::IAppearanceConfig, Microsoft::Terminal::Core::Color, CursorColor, nullptr);
        INHERITABLE_SETTING(Model::IAppearanceConfig, float, Opacity, 1.0f);

        INHERITABLE_SETTING(Model::IAppearanceConfig, hstring, DarkColorSchemeName, L"Campbell");
        INHERITABLE_SETTING(Model::IAppearanceConfig, hstring, LightColorSchemeName, L"Campbell");

#define APPEARANCE_SETTINGS_INITIALIZE(type, name, jsonKey, ...) \
    INHERITABLE_SETTING(Model::IAppearanceConfig, type, name, ##__VA_ARGS__)
        MTSM_APPEARANCE_SETTINGS(APPEARANCE_SETTINGS_INITIALIZE)
#undef APPEARANCE_SETTINGS_INITIALIZE

// Finite values clamp consistently for JSON and projected setters; null clears inheritance.
#define RETRO_INTENSITY_INITIALIZE(type, name, jsonKey, defaultValue) \
    _BASE_INHERITABLE_SETTING(Model::IAppearanceConfig, std::optional<float>, name) \
public: \
    float name() const { return _get##name##Impl().value_or(defaultValue); } \
    void name(float value) { \
        if (!std::isfinite(value)) { throw winrt::hresult_invalid_argument(); } \
        _##name = std::clamp(value, 0.0f, 1.0f); \
    }
        MTSM_RETRO_INTENSITY_SETTINGS(RETRO_INTENSITY_INITIALIZE)
#undef RETRO_INTENSITY_INITIALIZE

#define RETRO_DIMENSION_INITIALIZE(type, name, jsonKey, defaultValue, minimum, maximum) \
    _BASE_INHERITABLE_SETTING(Model::IAppearanceConfig, std::optional<float>, name) \
public: \
    float name() const { return _get##name##Impl().value_or(defaultValue); } \
    void name(float value) { \
        if (!std::isfinite(value)) { throw winrt::hresult_invalid_argument(); } \
        _##name = std::clamp(value, minimum, maximum); \
    }
        MTSM_RETRO_DIMENSION_SETTINGS(RETRO_DIMENSION_INITIALIZE)
#undef RETRO_DIMENSION_INITIALIZE

        _BASE_INHERITABLE_SETTING(Model::IAppearanceConfig, std::optional<Microsoft::Terminal::Core::Color>, RetroTerminalTintColor)
    public:
        Microsoft::Terminal::Core::Color RetroTerminalTintColor() const
        {
            return _getRetroTerminalTintColorImpl().value_or(Microsoft::Terminal::Core::Color{ 0, 255, 65, 255 });
        }
        void RetroTerminalTintColor(Microsoft::Terminal::Core::Color value)
        {
            // Phosphor is an RGB choice, never an opacity control.
            value.A = 255;
            _RetroTerminalTintColor = value;
        }

    private:
        winrt::weak_ref<Profile> _sourceProfile;
        std::set<std::string> _changeLog;

        void _logSettingSet(const std::string_view& setting);
        void _logSettingIfSet(const std::string_view& setting, const bool isSet);

        std::tuple<winrt::hstring, Model::OriginTag> _getSourceProfileBasePathAndOrigin() const;
    };
}
