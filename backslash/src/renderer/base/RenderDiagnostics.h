// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <atomic>
#include <cstdio>
#include <cstdint>
#include <memory>
#include <vector>
#include <wil/resource.h>

namespace Microsoft::Console::Render
{
    // Explicit developer opt-in only. Never records terminal text, paths or credentials.
    struct RenderDiagnostics
    {
        static std::shared_ptr<RenderDiagnostics> FromEnvironment()
        {
            constexpr auto variable = L"BACKSLASH_RENDER_DIAGNOSTICS";
            const auto length = GetEnvironmentVariableW(variable, nullptr, 0);
            if (length < 4 || length > 32768)
            {
                return {};
            }
            std::vector<wchar_t> path(length);
            const auto copied = GetEnvironmentVariableW(variable, path.data(), length);
            if (!copied || copied >= length)
            {
                return {};
            }
            const auto absolute = (path[1] == L':' && (path[2] == L'\\' || path[2] == L'/')) || (path[0] == L'\\' && path[1] == L'\\');
            if (!absolute)
            {
                return {};
            }
            wil::unique_hfile file{ CreateFileW(path.data(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr) };
            if (!file)
            {
                return {};
            }
            auto result = std::make_shared<RenderDiagnostics>();
            result->_file = std::move(file);
            return result;
        }

        std::atomic<uint64_t> outputCharacters{ 0 };
        std::atomic<uint64_t> paintRequests{ 0 };
        std::atomic<uint64_t> paintAttempts{ 0 };
        std::atomic<uint64_t> paintedFrames{ 0 };
        std::atomic<uint64_t> cleanFrames{ 0 };
        std::atomic<uint64_t> presents{ 0 };
        std::atomic<uint64_t> postprocessDraws{ 0 };
        std::atomic<uint64_t> shaderCompiles{ 0 };
        std::atomic<uint64_t> shaderRecreates{ 0 };
        std::atomic<uint64_t> shaderImageLoads{ 0 };
        std::atomic<uint64_t> shaderImageReleases{ 0 };
        std::atomic<uint64_t> textureCreates{ 0 };
        std::atomic<uint64_t> textureReleases{ 0 };
        std::atomic<uint64_t> offscreenBytes{ 0 };

        void Write(const char* reason, const bool visible, const bool suspended) noexcept
        {
            if (!_file)
            {
                return;
            }
            char line[1024];
            const auto size = std::snprintf(line, sizeof(line),
                "{\"pid\":%lu,\"rendererId\":%llu,\"tick\":%llu,\"reason\":\"%s\",\"visible\":%s,\"suspended\":%s,\"outputCharacters\":%llu,\"paintRequests\":%llu,\"paintAttempts\":%llu,\"paintedFrames\":%llu,\"cleanFrames\":%llu,\"presents\":%llu,\"postprocessDraws\":%llu,\"shaderCompiles\":%llu,\"shaderRecreates\":%llu,\"shaderImageLoads\":%llu,\"shaderImageReleases\":%llu,\"textureCreates\":%llu,\"textureReleases\":%llu,\"offscreenBytes\":%llu}\n",
                GetCurrentProcessId(), _id, GetTickCount64(), reason, visible ? "true" : "false", suspended ? "true" : "false",
                outputCharacters.load(std::memory_order_relaxed), paintRequests.load(std::memory_order_relaxed), paintAttempts.load(std::memory_order_relaxed), paintedFrames.load(std::memory_order_relaxed), cleanFrames.load(std::memory_order_relaxed),
                presents.load(std::memory_order_relaxed), postprocessDraws.load(std::memory_order_relaxed), shaderCompiles.load(std::memory_order_relaxed), shaderRecreates.load(std::memory_order_relaxed),
                shaderImageLoads.load(std::memory_order_relaxed), shaderImageReleases.load(std::memory_order_relaxed), textureCreates.load(std::memory_order_relaxed), textureReleases.load(std::memory_order_relaxed), offscreenBytes.load(std::memory_order_relaxed));
            if (size > 0 && static_cast<size_t>(size) < sizeof(line))
            {
                // One append write per snapshot, serialized across renderers in this module.
                const auto guard = _writeLock.lock_exclusive();
                DWORD written{};
                WriteFile(_file.get(), line, static_cast<DWORD>(size), &written, nullptr);
            }
        }

        void Sample(const bool visible, const bool suspended) noexcept
        {
            const auto now = GetTickCount64();
            if (now - _lastSample >= 1000)
            {
                _lastSample = now;
                Write("sample", visible, suspended);
            }
        }

    private:
        inline static std::atomic<uint64_t> _nextId{ 1 };
        inline static wil::srwlock _writeLock;
        const uint64_t _id = _nextId.fetch_add(1, std::memory_order_relaxed);
        // Only the renderer thread calls Sample; transitions use Write directly.
        uint64_t _lastSample = 0;
        wil::unique_hfile _file;
    };
}
