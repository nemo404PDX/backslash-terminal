// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include "SftpProtocol.h"
#include <array>
namespace Microsoft::Terminal::Backslash::Transfers::Sftp
{
    struct Progress { uint64_t completed; uint64_t total; };
    using ProgressCallback = std::function<void(Progress)>;
    using CancelCheck = std::function<void()>; // Throws when canceled; never retries.
    // Caller creates an exclusive temporary destination. Publication is separate:
    // interruption never truncates the final destination or deletes the source.
    inline void Download(Client& client, std::string_view source,
                         const std::function<void(std::string_view)>& writeTemporary,
                         const ProgressCallback& progress, const CancelCheck& cancel)
    {
        cancel();
        const auto attributes = client.Stat(source);
        if (!attributes.Regular() || !attributes.hasSize) throw std::runtime_error("Download requires a regular file with a known size");
        auto handle = client.Open(source,false,false);
        uint64_t offset=0;
        try {
            progress({0,attributes.size});
            for (;;) {
                cancel(); auto data=client.ReadChunk(handle,offset);
                if(data.empty()) break;
                if(offset>attributes.size || data.size()>attributes.size-offset) throw std::runtime_error("Remote file grew during download; temporary copy retained");
                writeTemporary(data); offset+=data.size(); progress({offset,attributes.size});
            }
            if(offset!=attributes.size) throw std::runtime_error("Remote file changed during download; temporary copy retained");
        } catch (...) { try { client.Close(handle); } catch (...) {} throw; }
        client.Close(handle); cancel();
        // Size validation is not a cryptographic integrity assertion.
    }
    inline void Upload(Client& client, std::string_view temporaryRemote, uint64_t size,
                       const std::function<std::string()>& readLocal,
                       const ProgressCallback& progress, const CancelCheck& cancel)
    {
        cancel(); auto handle=client.Open(temporaryRemote,true,false); // CREATE | EXCL
        uint64_t offset=0;
        try {
            progress({0,size});
            for (;;) {
                cancel(); auto data=readLocal(); if(data.empty()) break;
                if(offset>size || data.size()>size-offset) throw std::runtime_error("Local source grew during upload");
                client.WriteChunk(handle,offset,data); offset+=data.size(); progress({offset,size});
            }
            if(offset!=size) throw std::runtime_error("Local source changed during upload");
        } catch (...) { try { client.Close(handle); } catch (...) {} throw; }
        client.Close(handle); cancel();
        const auto attributes=client.Stat(temporaryRemote);
        if(!attributes.Regular() || !attributes.hasSize || attributes.size!=size) throw std::runtime_error("Remote temporary file size verification failed");
        // Worker owns the exact temporary path and offers explicit publication;
        // no deletion, overwrite, resume or retry is implied by this operation.
    }
}
