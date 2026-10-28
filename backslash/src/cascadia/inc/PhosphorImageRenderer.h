// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <windows.ui.composition.interop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <winrt/Windows.UI.Composition.h>

namespace Microsoft::Console::Render::Atlas
{
    // Uses the built-in shader bytecode, not runtime shader compilation.
    void CreateImagePhosphorShaders(ID3D11Device* device, ID3D11VertexShader** vertex, ID3D11PixelShader** pixel);
}

namespace Microsoft::Terminal::Backslash
{
    namespace ImageW = winrt::Windows;
    class PhosphorImageSource;

    // UI-thread-affine presentation. Configure/Refresh/Close and destruction
    // belong to the source visual's XAML dispatcher thread. Capture events use
    // that same dispatcher; the recursive lock also serializes D3D state.
    class PhosphorImagePresentation : public std::enable_shared_from_this<PhosphorImagePresentation>
    {
    public:
        ~PhosphorImagePresentation() { Close(); }
        ImageW::UI::Composition::SpriteVisual Visual() const { return _visual; }
        bool Ready() const noexcept { return _ready; }
        HRESULT LastError() const noexcept { return _error; }
        void SetChanged(std::function<void()> changed) { _changed = std::move(changed); }
        void Configure(ImageW::Foundation::Rect sourcePixels,
                       ImageW::Foundation::Size outputPixels,
                       ImageW::Foundation::Size outputDips,
                       ImageW::UI::Color endpoint,
                       float strength, float opacity, bool active);
        void Close() noexcept;

    private:
        friend class PhosphorImageSource;
        PhosphorImagePresentation(std::shared_ptr<PhosphorImageSource> source, ImageW::UI::Composition::Compositor compositor) :
            _source(std::move(source)), _compositor(std::move(compositor)), _visual(_compositor.CreateSpriteVisual())
        {
            _visual.Opacity(0);
        }
        void _render();
        void _reset() noexcept;
        void _state(bool ready, HRESULT error) noexcept;
        void _notify() noexcept { if (_changed) { try { _changed(); } catch (...) {} } }
        bool _active() const noexcept { return !_closed && _enabled && _opacity > 0 && _outputWidth > 0 && _outputHeight > 0 && _crop.Width > 0 && _crop.Height > 0; }

        std::shared_ptr<PhosphorImageSource> _source;
        ImageW::UI::Composition::Compositor _compositor{ nullptr };
        ImageW::UI::Composition::SpriteVisual _visual{ nullptr };
        ImageW::Foundation::Rect _crop{};
        ImageW::UI::Color _endpoint{ 255, 0, 255, 65 };
        UINT _outputWidth = 0, _outputHeight = 0, _inputWidth = 0, _inputHeight = 0;
        UINT _targetWidth = 0, _targetHeight = 0;
        ImageW::Foundation::Size _outputDips{};
        float _strength = 0, _opacity = 1;
        bool _enabled = false, _closed = false, _ready = false;
        HRESULT _error = S_OK;
        std::function<void()> _changed;
        winrt::com_ptr<ID3D11Texture2D> _input;
        winrt::com_ptr<ID3D11ShaderResourceView> _inputView;
        winrt::com_ptr<ID3D11RenderTargetView> _inputTarget, _target;
        winrt::com_ptr<IDXGISwapChain1> _swapChain;
        ImageW::UI::Composition::ICompositionSurface _surface{ nullptr };
    };

    class PhosphorImageSource : public std::enable_shared_from_this<PhosphorImageSource>
    {
    public:
        static std::shared_ptr<PhosphorImageSource> Acquire(ImageW::UI::Composition::Visual const& visual)
        {
            if (!visual) { winrt::throw_hresult(E_INVALIDARG); }
            static std::mutex registryMutex;
            static std::map<std::pair<void*, DWORD>, std::weak_ptr<PhosphorImageSource>> registry;
            const auto identity = visual.as<::IUnknown>();
            const auto key = std::pair<void*, DWORD>{ identity.get(), GetCurrentThreadId() };
            std::lock_guard lock{ registryMutex };
            for (auto it = registry.begin(); it != registry.end();)
            {
                if (it->second.expired()) { it = registry.erase(it); }
                else { ++it; }
            }
            if (const auto it = registry.find(key); it != registry.end())
            {
                if (auto source = it->second.lock(); source && !source->_closed) { return source; }
            }
            auto source = std::shared_ptr<PhosphorImageSource>{ new PhosphorImageSource{ visual } };
            registry[key] = source;
            return source;
        }

        ~PhosphorImageSource() { Close(); }
        std::shared_ptr<PhosphorImagePresentation> CreatePresentation(ImageW::UI::Composition::Compositor const& compositor)
        {
            std::lock_guard lock{ _mutex };
            if (_closed) { winrt::throw_hresult(RO_E_CLOSED); }
            auto presentation = std::shared_ptr<PhosphorImagePresentation>{ new PhosphorImagePresentation{ shared_from_this(), compositor } };
            _presentations.emplace_back(presentation);
            return presentation;
        }
        ImageW::Graphics::SizeInt32 PixelSize() const noexcept
        {
            std::lock_guard lock{ _mutex };
            if (!_closed && (_size.Width <= 0 || _size.Height <= 0))
            {
                try { const_cast<PhosphorImageSource*>(this)->_metadata(); } catch (...) {}
            }
            return _size;
        }
        HRESULT LastError() const noexcept { std::lock_guard lock{ _mutex }; return _error; }
        bool Capturing() const noexcept { std::lock_guard lock{ _mutex }; return static_cast<bool>(_session); }
        uint64_t DrawCount() const noexcept { std::lock_guard lock{ _mutex }; return _draws; }
        uint64_t FrameCount() const noexcept { std::lock_guard lock{ _mutex }; return _frames; }
        uint64_t CaptureStartCount() const noexcept { std::lock_guard lock{ _mutex }; return _captureStarts; }
        void Continuous(bool continuous) noexcept
        {
            std::lock_guard lock{ _mutex };
            _continuous = continuous;
            try { _start(); } catch (...) { _failed(winrt::to_hresult()); }
        }
        uint64_t EstimatedTextureBytes() const noexcept
        {
            std::lock_guard lock{ _mutex };
            uint64_t bytes = _lastFrame ? static_cast<uint64_t>(_frameSize.Width) * _frameSize.Height * 4 : 0;
            if (_pool) { bytes += static_cast<uint64_t>(_poolSize.Width) * _poolSize.Height * 8; }
            for (auto const& weak : _presentations)
            {
                if (auto p = weak.lock())
                {
                    if (p->_input) { bytes += static_cast<uint64_t>(p->_inputWidth) * p->_inputHeight * 4; }
                    if (p->_swapChain) { bytes += static_cast<uint64_t>(p->_targetWidth) * p->_targetHeight * 8; }
                }
            }
            return bytes;
        }
        void Refresh(bool discardCachedFrame = false) noexcept
        {
            std::lock_guard lock{ _mutex };
            if (_closed) { return; }
            try
            {
                if (discardCachedFrame)
                {
                    _stop();
                    _lastFrame = nullptr;
                    _size = {};
                    _captureRequested = true;
                    for (auto const& p : _live()) { p->_state(false, S_OK); }
                }
                const bool changed = _metadata();
                _start();
                if (changed) { for (auto const& p : _live()) { p->_notify(); } }
            }
            catch (...) { _failed(winrt::to_hresult()); }
        }
        void Close() noexcept
        {
            std::lock_guard lock{ _mutex };
            if (_closed) { return; }
            _closed = true;
            _stop();
            for (auto const& p : _live()) { p->_reset(); p->_state(false, RO_E_CLOSED); }
            _lastFrame = nullptr;
            _frameSize = {};
            _size = {};
            _item = nullptr;
            _visual = nullptr;
            _releaseDevice();
        }

    private:
        friend class PhosphorImagePresentation;
        friend struct PhosphorImageRendererTest;
        explicit PhosphorImageSource(ImageW::UI::Composition::Visual visual) : _visual(std::move(visual))
        {
            try { _metadata(); } catch (...) { _error = winrt::to_hresult(); }
        }
        bool _metadata()
        {
            if (_closed) { return false; }
            auto item = ImageW::Graphics::Capture::GraphicsCaptureItem::CreateFromVisual(_visual);
            const auto size = item.Size();
            if (size.Width <= 0 || size.Height <= 0)
            {
                if (!_session) { _error = S_FALSE; }
                return false;
            }
            const bool changed = _size.Width != size.Width || _size.Height != size.Height;
            _size = size;
            if (changed) { _captureRequested = true; }
            if (!_session) { _item = std::move(item); }
            return changed;
        }
        std::vector<std::shared_ptr<PhosphorImagePresentation>> _live()
        {
            std::vector<std::shared_ptr<PhosphorImagePresentation>> result;
            for (auto it = _presentations.begin(); it != _presentations.end();)
            {
                if (auto p = it->lock()) { result.emplace_back(std::move(p)); ++it; }
                else { it = _presentations.erase(it); }
            }
            return result;
        }
        void _deviceInit()
        {
            if (_device) { return; }
            winrt::check_hresult(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                                  nullptr, 0, D3D11_SDK_VERSION, _device.put(), nullptr, _context.put()));
            ::Microsoft::Console::Render::Atlas::CreateImagePhosphorShaders(_device.get(), _vertex.put(), _pixel.put());
            D3D11_SAMPLER_DESC sampler{};
            sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            sampler.MaxLOD = D3D11_FLOAT32_MAX;
            winrt::check_hresult(_device->CreateSamplerState(&sampler, _sampler.put()));
            D3D11_BUFFER_DESC buffer{};
            buffer.Usage = D3D11_USAGE_DEFAULT;
            buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            buffer.ByteWidth = 32;
            winrt::check_hresult(_device->CreateBuffer(&buffer, nullptr, _baseConstants.put()));
            buffer.ByteWidth = 48;
            winrt::check_hresult(_device->CreateBuffer(&buffer, nullptr, _retroConstants.put()));
            const auto dxgi = _device.as<IDXGIDevice>();
            ImageW::Foundation::IInspectable inspectable{ nullptr };
            winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi.get(), reinterpret_cast<IInspectable**>(winrt::put_abi(inspectable))));
            _captureDevice = inspectable.as<ImageW::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
        }
        void _start()
        {
            auto live = _live();
            const bool active = std::any_of(live.begin(), live.end(), [](auto const& p) { return p->_active(); });
            if (!active)
            {
                _stop();
                for (auto const& p : live) { p->_reset(); p->_state(false, S_OK); }
                _releaseDevice();
                return;
            }
            if (_session || _closed) { return; }
            _metadata();
            if (!_continuous && _lastFrame && !_captureRequested) { return; }
            const auto size = _size;
            // XAML may not have committed its first image/layout yet. Root calls
            // Refresh after ImageOpened/SizeChanged rather than starting a timer.
            if (size.Width <= 0 || size.Height <= 0) { _error = S_FALSE; return; }
            _deviceInit();
            _pool = ImageW::Graphics::Capture::Direct3D11CaptureFramePool::Create(_captureDevice,
                ImageW::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, size);
            const auto weak = weak_from_this();
            _frameToken = _pool.FrameArrived([weak](auto const&, auto const&) { if (auto source = weak.lock()) { source->_arrived(); } });
            _session = _pool.CreateCaptureSession(_item);
            _poolSize = size;
            _session.StartCapture();
            ++_captureStarts;
            _error = S_OK;
        }
        void _stop() noexcept
        {
            try { if (_pool) { _pool.FrameArrived(_frameToken); } } catch (...) {}
            try { if (_session) { _session.Close(); } } catch (...) {}
            try { if (_pool) { _pool.Close(); } } catch (...) {}
            _session = nullptr;
            _pool = nullptr;
            _frameToken = {};
        }
        void _releaseDevice() noexcept
        {
            _lastFrame = nullptr;
            _frameSize = {};
            _captureRequested = true;
            _captureDevice = nullptr;
            _vertex = nullptr;
            _pixel = nullptr;
            _sampler = nullptr;
            _baseConstants = nullptr;
            _retroConstants = nullptr;
            if (_context) { _context->ClearState(); }
            _context = nullptr;
            _device = nullptr;
        }
        void _failed(HRESULT error) noexcept
        {
            _error = error;
            _stop();
            _releaseDevice();
            for (auto const& p : _live()) { p->_reset(); p->_state(false, error); }
        }
        void _arrived() noexcept
        {
            std::lock_guard lock{ _mutex };
            if (_closed || !_pool) { return; }
            try
            {
                auto frame = _pool.TryGetNextFrame();
                if (!frame) { return; }
                // At most two frames are retained by the pool. Consume newest,
                // so a temporarily busy UI thread does not build up latency.
                while (auto newer = _pool.TryGetNextFrame()) { frame.Close(); frame = std::move(newer); }
                const auto content = frame.ContentSize();
                auto access = frame.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
                winrt::com_ptr<ID3D11Texture2D> texture;
                winrt::check_hresult(access->GetInterface(__uuidof(ID3D11Texture2D), texture.put_void()));
                D3D11_TEXTURE2D_DESC desc{};
                texture->GetDesc(&desc);
                const UINT width = std::min(desc.Width, static_cast<UINT>(std::max(0, content.Width)));
                const UINT height = std::min(desc.Height, static_cast<UINT>(std::max(0, content.Height)));
                if (!width || !height) { frame.Close(); return; }
                const bool changed = _frameSize.Width != static_cast<int32_t>(width) || _frameSize.Height != static_cast<int32_t>(height);
                if (!_lastFrame || changed)
                {
                    desc.Width = width;
                    desc.Height = height;
                    desc.MipLevels = desc.ArraySize = 1;
                    desc.Usage = D3D11_USAGE_DEFAULT;
                    desc.BindFlags = 0;
                    desc.CPUAccessFlags = desc.MiscFlags = 0;
                    winrt::check_hresult(_device->CreateTexture2D(&desc, nullptr, _lastFrame.put()));
                    _frameSize = { static_cast<int32_t>(width), static_cast<int32_t>(height) };
                }
                _size = content;
                const D3D11_BOX region{ 0, 0, 0, width, height, 1 };
                _context->CopySubresourceRegion(_lastFrame.get(), 0, 0, 0, 0, texture.get(), 0, &region);
                ++_frames;
                _captureRequested = false;
                frame.Close();
                if (content.Width > 0 && content.Height > 0 &&
                    (_poolSize.Width != content.Width || _poolSize.Height != content.Height))
                {
                    _pool.Recreate(_captureDevice, ImageW::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, content);
                    _poolSize = content;
                }
                _error = S_OK;
                for (auto const& p : _live())
                {
                    if (p->_active()) { p->_render(); }
                    if (changed) { p->_notify(); }
                }
                if (!_continuous) { _stop(); }
            }
            catch (...) { _failed(winrt::to_hresult()); }
        }

        mutable std::recursive_mutex _mutex;
        ImageW::UI::Composition::Visual _visual{ nullptr };
        std::vector<std::weak_ptr<PhosphorImagePresentation>> _presentations;
        bool _closed = false;
        bool _continuous = true, _captureRequested = true;
        HRESULT _error = S_OK;
        ImageW::Graphics::SizeInt32 _size{};
        ImageW::Graphics::SizeInt32 _frameSize{};
        ImageW::Graphics::SizeInt32 _poolSize{};
        uint64_t _draws = 0, _frames = 0, _captureStarts = 0;
        ImageW::Graphics::Capture::GraphicsCaptureItem _item{ nullptr };
        ImageW::Graphics::DirectX::Direct3D11::IDirect3DDevice _captureDevice{ nullptr };
        ImageW::Graphics::Capture::Direct3D11CaptureFramePool _pool{ nullptr };
        ImageW::Graphics::Capture::GraphicsCaptureSession _session{ nullptr };
        winrt::event_token _frameToken{};
        winrt::com_ptr<ID3D11Device> _device;
        winrt::com_ptr<ID3D11DeviceContext> _context;
        winrt::com_ptr<ID3D11Texture2D> _lastFrame;
        winrt::com_ptr<ID3D11VertexShader> _vertex;
        winrt::com_ptr<ID3D11PixelShader> _pixel;
        winrt::com_ptr<ID3D11SamplerState> _sampler;
        winrt::com_ptr<ID3D11Buffer> _baseConstants, _retroConstants;
    };

    inline void PhosphorImagePresentation::_state(bool ready, HRESULT error) noexcept
    {
        const bool changed = _ready != ready || _error != error;
        _ready = ready;
        _error = error;
        try { _visual.Opacity(ready && _active() ? _opacity : 0); } catch (...) {}
        if (changed) { _notify(); }
    }
    inline void PhosphorImagePresentation::_reset() noexcept
    {
        _input = nullptr;
        _inputView = nullptr;
        _inputTarget = nullptr;
        _target = nullptr;
        _swapChain = nullptr;
        _surface = nullptr;
        _inputWidth = _inputHeight = _targetWidth = _targetHeight = 0;
        try { _visual.Brush(nullptr); } catch (...) {}
        try { _visual.Opacity(0); } catch (...) {}
    }
    inline void PhosphorImagePresentation::Configure(ImageW::Foundation::Rect sourcePixels,
                                                    ImageW::Foundation::Size outputPixels,
                                                    ImageW::Foundation::Size outputDips,
                                                    ImageW::UI::Color endpoint,
                                                    float strength, float opacity, bool active)
    {
        const auto source = _source;
        if (!source || _closed) { return; }
        std::lock_guard lock{ source->_mutex };
        if (source->_closed) { _state(false, RO_E_CLOSED); return; }
        try
        {
            const auto valid = [](float v) { return std::isfinite(v); };
            if (!valid(sourcePixels.X) || !valid(sourcePixels.Y) || !valid(sourcePixels.Width) || !valid(sourcePixels.Height) ||
                !valid(outputPixels.Width) || !valid(outputPixels.Height) || !valid(outputDips.Width) || !valid(outputDips.Height) ||
                !valid(strength) || !valid(opacity)) { winrt::throw_hresult(E_INVALIDARG); }
            if (sourcePixels.Width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || sourcePixels.Height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
                outputPixels.Width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || outputPixels.Height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
                std::abs(sourcePixels.X) > 1000000000.0f || std::abs(sourcePixels.Y) > 1000000000.0f)
            { winrt::throw_hresult(E_INVALIDARG); }
            const auto outputWidth = static_cast<UINT>(std::ceil(std::max(0.0f, outputPixels.Width)));
            const auto outputHeight = static_cast<UINT>(std::ceil(std::max(0.0f, outputPixels.Height)));
            strength = std::clamp(strength, 0.0f, 1.0f);
            opacity = std::clamp(opacity, 0.0f, 1.0f);
            active = active && outputDips.Width > 0 && outputDips.Height > 0;
            const bool changed = _crop.X != sourcePixels.X || _crop.Y != sourcePixels.Y || _crop.Width != sourcePixels.Width || _crop.Height != sourcePixels.Height ||
                _outputWidth != outputWidth || _outputHeight != outputHeight || _strength != strength ||
                _endpoint.R != endpoint.R || _endpoint.G != endpoint.G || _endpoint.B != endpoint.B ||
                _enabled != active || _outputDips.Width != outputDips.Width || _outputDips.Height != outputDips.Height;
            _crop = sourcePixels;
            _outputWidth = outputWidth;
            _outputHeight = outputHeight;
            _endpoint = endpoint;
            _strength = strength;
            _opacity = opacity;
            _enabled = active;
            _outputDips = outputDips;
            _visual.Size({ std::max(0.0f, outputDips.Width), std::max(0.0f, outputDips.Height) });
            _visual.Opacity(_ready && _active() ? _opacity : 0);
            source->_start();
            if (!_active()) { _reset(); _state(false, S_OK); }
            else if (source->_lastFrame && (changed || !_ready)) { _render(); }
        }
        catch (...) { source->_failed(winrt::to_hresult()); }
    }
    inline void PhosphorImagePresentation::Close() noexcept
    {
        const auto source = _source;
        if (!source || _closed) { return; }
        std::lock_guard lock{ source->_mutex };
        _closed = true;
        _changed = {};
        _reset();
        _ready = false;
        _source.reset();
        try { source->_start(); } catch (...) { source->_failed(winrt::to_hresult()); }
    }
    inline void PhosphorImagePresentation::_render()
    {
        if (!_active() || !_source || _source->_closed || !_source->_lastFrame) { return; }
        auto& source = *_source;
        const auto device = source._device.get();
        const auto context = source._context.get();
        const float fractionX = _crop.X - std::floor(_crop.X), fractionY = _crop.Y - std::floor(_crop.Y);
        const UINT width = static_cast<UINT>(std::ceil(_crop.Width + fractionX));
        const UINT height = static_cast<UINT>(std::ceil(_crop.Height + fractionY));
        if (width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION) { winrt::throw_hresult(E_INVALIDARG); }
        if (!_input || width != _inputWidth || height != _inputHeight)
        {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = width;
            desc.Height = height;
            desc.MipLevels = desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            winrt::check_hresult(device->CreateTexture2D(&desc, nullptr, _input.put()));
            winrt::check_hresult(device->CreateShaderResourceView(_input.get(), nullptr, _inputView.put()));
            winrt::check_hresult(device->CreateRenderTargetView(_input.get(), nullptr, _inputTarget.put()));
            _inputWidth = width;
            _inputHeight = height;
        }
        if (!_swapChain || _targetWidth != _outputWidth || _targetHeight != _outputHeight)
        {
            _target = nullptr;
            _surface = nullptr;
            _swapChain = nullptr;
            const auto dxgi = source._device.as<IDXGIDevice>();
            winrt::com_ptr<IDXGIAdapter> adapter;
            winrt::check_hresult(dxgi->GetAdapter(adapter.put()));
            winrt::com_ptr<IDXGIFactory2> factory;
            winrt::check_hresult(adapter->GetParent(__uuidof(IDXGIFactory2), factory.put_void()));
            DXGI_SWAP_CHAIN_DESC1 desc{};
            desc.Width = _outputWidth;
            desc.Height = _outputHeight;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.BufferCount = 2;
            desc.Scaling = DXGI_SCALING_STRETCH;
            desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
            winrt::check_hresult(factory->CreateSwapChainForComposition(device, &desc, nullptr, _swapChain.put()));
            winrt::com_ptr<ID3D11Texture2D> target;
            winrt::check_hresult(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), target.put_void()));
            winrt::check_hresult(device->CreateRenderTargetView(target.get(), nullptr, _target.put()));
            winrt::check_hresult(_compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>()->CreateCompositionSurfaceForSwapChain(
                _swapChain.get(), reinterpret_cast<ABI::Windows::UI::Composition::ICompositionSurface**>(winrt::put_abi(_surface))));
            _visual.Brush(_compositor.CreateSurfaceBrush(_surface));
            _targetWidth = _outputWidth;
            _targetHeight = _outputHeight;
        }
        const float clear[4]{};
        context->ClearRenderTargetView(_inputTarget.get(), clear);
        const int x = static_cast<int>(std::floor(_crop.X)), y = static_cast<int>(std::floor(_crop.Y));
        const int left = std::max(0, x), top = std::max(0, y);
        const int right = std::min(source._frameSize.Width, x + static_cast<int>(width));
        const int bottom = std::min(source._frameSize.Height, y + static_cast<int>(height));
        if (right > left && bottom > top)
        {
            const D3D11_BOX box{ static_cast<UINT>(left), static_cast<UINT>(top), 0, static_cast<UINT>(right), static_cast<UINT>(bottom), 1 };
            context->CopySubresourceRegion(_input.get(), 0, static_cast<UINT>(left - x), static_cast<UINT>(top - y), 0, source._lastFrame.get(), 0, &box);
        }
        const float base[8]{ 0, 1, static_cast<float>(_outputWidth), static_cast<float>(_outputHeight), 0, 0, 0, 0 };
        const float retro[12]{ 0, 0, 4, 4, _endpoint.R / 255.0f, _endpoint.G / 255.0f, _endpoint.B / 255.0f, _strength, 0, 0, 1, 0 };
        context->UpdateSubresource(source._baseConstants.get(), 0, nullptr, base, 0, 0);
        context->UpdateSubresource(source._retroConstants.get(), 0, nullptr, retro, 0, 0);
        ID3D11Buffer* buffers[]{ source._baseConstants.get(), source._retroConstants.get() };
        context->IASetInputLayout(nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        context->VSSetShader(source._vertex.get(), nullptr, 0);
        context->PSSetShader(source._pixel.get(), nullptr, 0);
        context->PSSetConstantBuffers(0, 2, buffers);
        auto resource = _inputView.get();
        context->PSSetShaderResources(0, 1, &resource);
        auto sampler = source._sampler.get();
        context->PSSetSamplers(0, 1, &sampler);
        const D3D11_VIEWPORT viewport{
            -fractionX * _outputWidth / _crop.Width, -fractionY * _outputHeight / _crop.Height,
            width * static_cast<float>(_outputWidth) / _crop.Width, height * static_cast<float>(_outputHeight) / _crop.Height, 0, 1 };
        context->RSSetViewports(1, &viewport);
        auto target = _target.get();
        context->ClearRenderTargetView(target, clear);
        context->OMSetRenderTargets(1, &target, nullptr);
        context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
        context->Draw(4, 0);
        context->OMSetRenderTargets(0, nullptr, nullptr);
        ID3D11ShaderResourceView* empty = nullptr;
        context->PSSetShaderResources(0, 1, &empty);
        winrt::check_hresult(_swapChain->Present(0, 0));
        ++source._draws;
        _state(true, S_OK);
    }
}
