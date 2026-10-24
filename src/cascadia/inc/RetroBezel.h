// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once

#include "RetroBezelGeometry.h"
#include "BezelBadgeMaterial.h"
#include "BezelBadgeLayout.h"
#include "BezelBadgeSvg.h"
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Automation.h>
#include <winrt/Windows.UI.Xaml.Automation.Peers.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>

namespace Microsoft::Terminal::Backslash
{
    // Shared by live panes and the appearance preview. Decoration never receives input.
    // The entire TermControl (including its scrollbar/overlays) is inside the aperture.
    class RetroBezel
    {
        using Grid = winrt::Windows::UI::Xaml::Controls::Grid;
        using Border = winrt::Windows::UI::Xaml::Controls::Border;
        using Image = winrt::Windows::UI::Xaml::Controls::Image;
        using Color = winrt::Windows::UI::Color;
        using Thickness = winrt::Windows::UI::Xaml::Thickness;
        Grid _root, _frame;
        winrt::Windows::UI::Xaml::Controls::Canvas _materialCanvas;
        winrt::Windows::UI::Xaml::Shapes::Path _shell, _aperture;
        Border _surround, _rim;
        Image _badge;
        int _badgePlacement = 0, _badgeStyle = 0, _loadedBadgeStyle = -1;
        std::wstring _badgeLettering{ L"#FFFFFF" }, _badgeBacking{ L"#000000" };
        BezelInsets _frameInsets{};
        bool _enabled = false;
        winrt::Windows::UI::Xaml::FrameworkElement _content{ nullptr };

        static auto _gradient(Color first, Color last, BezelGradientDirection direction = { 0, 0, 1, 1 })
        {
            using namespace winrt::Windows::UI::Xaml::Media;
            LinearGradientBrush brush;
            brush.StartPoint({ direction.startX, direction.startY });
            brush.EndPoint({ direction.endX, direction.endY });
            GradientStop a, b;
            a.Offset(0); a.Color(first);
            b.Offset(1); b.Color(last);
            brush.GradientStops().Append(a);
            brush.GradientStops().Append(b);
            return brush;
        }

        static Color _color(std::uint32_t argb)
        {
            return { static_cast<uint8_t>(argb >> 24), static_cast<uint8_t>(argb >> 16),
                     static_cast<uint8_t>(argb >> 8), static_cast<uint8_t>(argb) };
        }

        static Color _mix(Color color, int target, float amount)
        {
            return { 255,
                     static_cast<uint8_t>(color.R + (target - color.R) * amount),
                     static_cast<uint8_t>(color.G + (target - color.G) * amount),
                     static_cast<uint8_t>(color.B + (target - color.B) * amount) };
        }

        void _updateShellGeometry()
        {
            using namespace winrt::Windows::UI::Xaml::Media;
            using Point = winrt::Windows::Foundation::Point;
            const auto width = static_cast<float>(_frame.ActualWidth());
            const auto height = static_cast<float>(_frame.ActualHeight());
            auto line = [](const PathFigure& figure, Point point) {
                LineSegment segment;
                segment.Point(point);
                figure.Segments().Append(segment);
            };
            auto rectangle = [&](float left, float top, float right, float bottom) {
                PathFigure figure;
                figure.StartPoint({ left, top });
                figure.IsClosed(true);
                line(figure, { right, top });
                line(figure, { right, bottom });
                line(figure, { left, bottom });
                return figure;
            };
            // The entire curved inner edge stays outside rectangular terminal cells.
            const auto aperture = GetBezelApertureBounds(width, height, _frameInsets);
            const auto left = aperture.opening.left;
            const auto top = aperture.opening.top;
            const auto right = aperture.opening.right;
            const auto bottom = aperture.opening.bottom;
            const auto radii = GetBezelRecessRadii(right - left, bottom - top);
            auto roundedRectangle = [&](float x1, float y1, float x2, float y2, float requestedRadius) {
                const auto radius = ClampBezelCornerRadius(requestedRadius, x2 - x1, y2 - y1);
                if (radius == 0) { return rectangle(x1, y1, x2, y2); }
                auto arc = [radius](const PathFigure& figure, Point point) {
                    ArcSegment segment;
                    segment.Point(point);
                    segment.Size({ radius, radius });
                    segment.SweepDirection(SweepDirection::Clockwise);
                    figure.Segments().Append(segment);
                };
                PathFigure inner;
                inner.StartPoint({ x1 + radius, y1 });
                inner.IsClosed(true);
                line(inner, { x2 - radius, y1 });
                arc(inner, { x2, y1 + radius });
                line(inner, { x2, y2 - radius });
                arc(inner, { x2 - radius, y2 });
                line(inner, { x1 + radius, y2 });
                arc(inner, { x1, y2 - radius });
                line(inner, { x1, y1 + radius });
                arc(inner, { x1 + radius, y1 });
                return inner;
            };
            PathGeometry casing, recess;
            casing.FillRule(FillRule::EvenOdd);
            recess.FillRule(FillRule::EvenOdd);
            if (width > 0 && height > 0)
            {
                casing.Figures().Append(rectangle(0, 0, width, height));
                if (_enabled && right > left && bottom > top)
                {
                    // Small outer dark-band corners, larger inner opening corners.
                    // The external casing and terminal rectangle are unchanged.
                    recess.Figures().Append(roundedRectangle(aperture.recess.left, aperture.recess.top,
                                                            aperture.recess.right, aperture.recess.bottom,
                                                            radii.outer));
                    casing.Figures().Append(roundedRectangle(left, top, right, bottom, radii.inner));
                    recess.Figures().Append(roundedRectangle(left, top, right, bottom, radii.inner));
                }
            }
            _shell.Data(casing);
            _aperture.Data(recess);
        }

        void _updateBadge()
        {
            using namespace winrt::Windows::UI::Xaml;
            const auto layout = GetBezelBadgeLayout(_enabled, _badgePlacement,
                                                    static_cast<float>(_frame.ActualWidth()),
                                                    static_cast<float>(_frame.ActualHeight()), _frameInsets);
            _badge.Visibility(layout.visible ? Visibility::Visible : Visibility::Collapsed);
            _badge.Width(layout.width);
            _badge.Height(layout.height);
            Controls::Canvas::SetLeft(_badge, layout.left);
            Controls::Canvas::SetTop(_badge, layout.top);
            if (_enabled && _badgePlacement >= 1 && _badgePlacement <= 6 && _loadedBadgeStyle != _badgeStyle)
            {
                Media::Imaging::SvgImageSource source;
                _badge.Source(source);
                _loadedBadgeStyle = _badgeStyle;
                LoadBezelBadgeSvg(source, _badgeStyle, _badgeLettering, _badgeBacking);
            }
        }

    public:
        BezelInsets Insets{};
        RetroBezel()
        {
            using namespace winrt::Windows::UI::Xaml;
            using namespace winrt::Windows::UI::Xaml::Controls;
            _root.Children().Append(_frame);
            // Hollow black backing covers every non-screen pixel, including the
            // rounded opening's safety gutter. No Background exists behind the screen.
            _surround.IsHitTestVisible(false);
            _surround.CornerRadius({ 0, 0, 0, 0 });
            _surround.Background(nullptr);
            _surround.Opacity(1);
            _surround.BorderBrush(Media::SolidColorBrush{ Color{
                static_cast<uint8_t>(BezelSurroundArgb >> 24),
                static_cast<uint8_t>(BezelSurroundArgb >> 16),
                static_cast<uint8_t>(BezelSurroundArgb >> 8),
                static_cast<uint8_t>(BezelSurroundArgb) } });
            _frame.Children().Append(_surround);
            _shell.IsHitTestVisible(false);
            _shell.Stretch(Media::Stretch::None);
            // Canvas contributes no desired size: resize geometry cannot feed back into grid sizing.
            _materialCanvas.IsHitTestVisible(false);
            _materialCanvas.Children().Append(_shell);
            _frame.Children().Append(_materialCanvas);
            _rim.IsHitTestVisible(false);
            _rim.CornerRadius({ BezelOuterRadius, BezelOuterRadius, BezelOuterRadius, BezelOuterRadius });
            _rim.BorderThickness({ BezelOuterRimThickness, BezelOuterRimThickness, BezelOuterRimThickness, BezelOuterRimThickness });
            _frame.Children().Append(_rim);
            _aperture.IsHitTestVisible(false);
            _aperture.Stretch(Media::Stretch::None);
            _materialCanvas.Children().Append(_aperture);
            _badge.IsHitTestVisible(false);
            _badge.Stretch(Media::Stretch::Uniform);
            Automation::AutomationProperties::SetAccessibilityView(_badge, Automation::Peers::AccessibilityView::Raw);
            _materialCanvas.Children().Append(_badge);
            _frame.SizeChanged([this](const auto&, const auto&) { _updateShellGeometry(); _updateBadge(); });
            Update(false, 1, 0, 0, false);
        }

        static Color PaletteColor(int color)
        {
            return _color(BezelPaletteArgb(color));
        }

        Grid Root() const { return _root; }
        void Content(winrt::Windows::UI::Xaml::FrameworkElement const& content)
        {
            _content = content;
            _frame.Children().Append(content);
        }

        void Update(bool enabled, int size, int color, int badge, bool center, bool fullscreen = false, int filler = 0, int badgeStyle = 0, std::wstring_view lettering = L"#FFFFFF", std::wstring_view backing = L"#000000", int top = -1, int right = -1, int bottom = -1, int left = -1)
        {
            using namespace winrt::Windows::UI::Xaml;
            // Balance inside the full-size material casing. An outer Grid margin
            // left an unpainted strip that showed wallpaper in transparent fullscreen hosts.
            Insets = GetBezelInsetsForMode(enabled, size, badge, center, fullscreen, filler, top, right, bottom, left);
            const auto frame = Insets;
            _frame.Margin({ 0, 0, 0, 0 });
            if (_content) { _content.Margin({ frame.left, frame.top, frame.right, frame.bottom }); }
            const auto visibility = enabled ? Visibility::Visible : Visibility::Collapsed;
            _surround.Visibility(visibility);
            // Same bounds as the content margin, so this border's hollow center
            // never paints beneath translucent terminal padding/cells/overlays.
            _surround.BorderThickness({ frame.left, frame.top, frame.right, frame.bottom });
            _shell.Visibility(visibility);
            _rim.Visibility(visibility);
            _enabled = enabled;
            _frameInsets = frame;
            _updateShellGeometry();
            _aperture.Visibility(visibility);
            _badgePlacement = badge;
            _badgeStyle = badgeStyle;
            if (_badgeLettering != lettering || _badgeBacking != backing)
            {
                _badgeLettering = lettering;
                _badgeBacking = backing;
                if (_badgeStyle == 2) { _loadedBadgeStyle = -1; }
            }
            _updateBadge();

            // One continuous brush per hollow ring keeps corners seamless and
            // leaves the rectangular terminal content outside all material paint.
            const auto flat = GetBezelCasingMaterial(color);
            _shell.Fill(_gradient(_color(flat.leftShadow), _color(flat.rightHighlight), BezelCasingGradient));
            if (IsDarkBezelPalette(color))
            {
                const auto casing = GetDarkBezelCasingMaterial(color);
                _rim.BorderBrush(_gradient(_color(casing.rimTop), _color(casing.rimBottom)));
            }
            else
            {
                // Retain the existing light-plastic outer-rim tones.
                const auto base = PaletteColor(color);
                _rim.BorderBrush(_gradient(_mix(base, 255, 0.65f), _mix(base, 0, 0.48f)));
            }
            const auto band = GetBezelRecessMaterial(color);
            _aperture.Fill(_gradient(_color(band.highlight), _color(band.shadow), BezelRecessGradient));

        }
    };
}
