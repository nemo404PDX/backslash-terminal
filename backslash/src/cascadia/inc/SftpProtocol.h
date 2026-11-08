// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>
namespace Microsoft::Terminal::Backslash::Transfers::Sftp
{
    using Bytes = std::vector<uint8_t>;
    inline constexpr size_t MaxPacket = 1024 * 1024;
    inline bool ValidUtf8(std::string_view value) noexcept
    {
        size_t i=0;
        while(i<value.size()) {
            const auto lead=static_cast<uint8_t>(value[i++]);
            if(lead<0x80) { if(!lead) return false; continue; }
            uint32_t code=0; size_t count=0; uint32_t minimum=0;
            if(lead>=0xc2&&lead<=0xdf) {code=lead&31;count=1;minimum=0x80;}
            else if(lead>=0xe0&&lead<=0xef) {code=lead&15;count=2;minimum=0x800;}
            else if(lead>=0xf0&&lead<=0xf4) {code=lead&7;count=3;minimum=0x10000;}
            else return false;
            if(count>value.size()-i) return false;
            for(size_t n=0;n<count;++n) {auto c=static_cast<uint8_t>(value[i++]);if((c&0xc0)!=0x80)return false;code=(code<<6)|(c&63);}
            if(code<minimum||code>0x10ffff||(code>=0xd800&&code<=0xdfff))return false;
        }
        return true;
    }
    inline void ValidatePath(std::string_view path)
    {
        if(path.empty() || path.size()>32768 || !ValidUtf8(path)) throw std::runtime_error("Remote path must be bounded UTF-8 without NUL");
    }
    inline std::string ParentPath(std::string_view path)
    {
        ValidatePath(path);const bool absolute=path.front()=='/';std::vector<std::string> parts;
        size_t begin=0;while(begin<=path.size()){const auto end=path.find('/',begin);const auto part=path.substr(begin,end==path.npos?path.size()-begin:end-begin);
            if(!part.empty()&&part!="."){if(part==".."&&!parts.empty()&&parts.back()!="..")parts.pop_back();else if(part!=".."||!absolute)parts.emplace_back(part);}
            if(end==path.npos)break;begin=end+1;
        }
        if(!parts.empty()&&parts.back()!="..")parts.pop_back();else if(!absolute)parts.push_back("..");
        std::string result=absolute?"/":"";for(const auto& part:parts){if(!result.empty()&&result.back()!='/')result+='/';result+=part;}return result.empty()?".":result;
    }
    inline bool SafeLocalName(std::string_view name)
    {
        if(name.empty()||name=="."||name==".."||!ValidUtf8(name)||name.find_first_of("<>:\"/\\|?*")!=name.npos||name.back()=='.'||name.back()==' ')return false;
        for(auto c:name)if(static_cast<uint8_t>(c)<32)return false;
        auto stem=std::string(name.substr(0,name.find('.')));for(auto& c:stem)if(c>='a'&&c<='z')c-=32;
        while(!stem.empty()&&stem.back()==' ')stem.pop_back();
        for(const auto* prefix:{"COM","LPT"})for(const auto* suffix:{"\xc2\xb9","\xc2\xb2","\xc2\xb3"})if(stem==std::string(prefix)+suffix)return false;
        if(stem=="CON"||stem=="PRN"||stem=="AUX"||stem=="NUL"||stem=="CONIN$"||stem=="CONOUT$")return false;
        if(stem.size()==4&&(stem.starts_with("COM")||stem.starts_with("LPT"))&&stem[3]>='1'&&stem[3]<='9')return false;
        return true;
    }
    struct Wire
    {
        Bytes bytes;
        size_t cursor = 0;
        void U32(uint32_t n) { for (int shift = 24; shift >= 0; shift -= 8) bytes.push_back(static_cast<uint8_t>(n >> shift)); }
        void U64(uint64_t n) { U32(static_cast<uint32_t>(n >> 32)); U32(static_cast<uint32_t>(n)); }
        uint32_t U32() { if (bytes.size() - cursor < 4) throw std::runtime_error("Truncated SFTP integer"); uint32_t n = 0; for (int i = 0; i < 4; ++i) n = (n << 8) | bytes[cursor++]; return n; }
        uint64_t U64() { const auto high = U32(); return (static_cast<uint64_t>(high) << 32) | U32(); }
        void String(std::string_view value) { if (value.size() > MaxPacket) throw std::runtime_error("SFTP string too large"); U32(static_cast<uint32_t>(value.size())); bytes.insert(bytes.end(), value.begin(), value.end()); }
        std::string String() { const auto n = U32(); if (n > bytes.size() - cursor) throw std::runtime_error("Truncated SFTP string"); std::string value(bytes.begin() + cursor, bytes.begin() + cursor + n); cursor += n; return value; }
        void End() const { if (cursor != bytes.size()) throw std::runtime_error("Unexpected SFTP response data"); }
    };
    struct Attributes
    {
        uint64_t size = 0;
        uint32_t permissions = 0;
        bool hasSize = false;
        bool hasPermissions = false;
        bool Directory() const { return hasPermissions && (permissions & 0170000) == 0040000; }
        bool Regular() const { return hasPermissions && (permissions & 0170000) == 0100000; }
    };
    inline Attributes ReadAttributes(Wire& wire)
    {
        Attributes a;
        const auto flags = wire.U32();
        if (flags & ~0x8000000fu) throw std::runtime_error("Unsupported SFTP attributes");
        if (flags & 1) { a.size = wire.U64(); a.hasSize = true; }
        if (flags & 2) { wire.U32(); wire.U32(); }
        if (flags & 4) { a.permissions = wire.U32(); a.hasPermissions = true; }
        if (flags & 8) { wire.U32(); wire.U32(); }
        if (flags & 0x80000000) { const auto count = wire.U32(); if (count > 1024) throw std::runtime_error("Too many SFTP attributes"); for (uint32_t i = 0; i < count; ++i) { wire.String(); wire.String(); } }
        return a;
    }
    struct StatusError : std::runtime_error { uint32_t code; explicit StatusError(uint32_t value) : std::runtime_error("SFTP request failed (status "+std::to_string(value)+")"),code(value){} };
    struct Entry { std::string name; Attributes attributes; };
    // Own one serialized session per worker. Transport supplies exact reads/writes,
    // cancellation and deadlines; it must never forward these bytes to a shell.
    class Client
    {
    public:
        using Read = std::function<void(std::span<uint8_t>)>;
        using Write = std::function<void(std::span<const uint8_t>)>;
        Client(Read read, Write write) : _read(std::move(read)), _write(std::move(write)) {}
        void Initialize()
        {
            Wire init; init.bytes.push_back(1); init.U32(3); Send(init);
            auto response = Receive();
            if (response.bytes.front() != 2 || response.U32() != 3) throw std::runtime_error("SFTP version 3 required");
            while (response.cursor < response.bytes.size()) { auto name = response.String(); auto value = response.String(); if (name == "posix-rename@openssh.com" && value == "1") _atomicReplace = true; }
        }
        bool AtomicReplace() const noexcept { return _atomicReplace; }
        void Rename(std::string_view source, std::string_view destination, bool replace)
        {
            if (replace && !_atomicReplace) throw std::runtime_error("Server does not support atomic replacement; choose a new name");
            ValidatePath(source); ValidatePath(destination);
            Wire request; if (replace) request.String("posix-rename@openssh.com"); request.String(source); request.String(destination);
            auto response = Request(replace ? 200 : 18, request, 101); CheckStatus(response);
        }
        void Remove(std::string_view path) { ValidatePath(path); Wire request; request.String(path); auto response = Request(13, request, 101); CheckStatus(response); }
        std::string Open(std::string_view path, bool upload, bool replace)
        {
            ValidatePath(path); Wire request; request.String(path); request.U32(upload ? (replace ? 0x1a : 0x2a) : 1); request.U32(0);
            auto response = Request(3, request, 102); auto handle = response.String(); response.End(); return handle;
        }
        void Close(std::string_view handle) { Wire request; request.String(handle); auto response = Request(4, request, 101); CheckStatus(response); }
        Attributes Stat(std::string_view path) { ValidatePath(path); Wire request; request.String(path); auto response = Request(7, request, 105); auto a = ReadAttributes(response); response.End(); return a; }
        std::vector<Entry> List(std::string_view path)
        {
            ValidatePath(path); Wire request; request.String(path); auto opened = Request(11, request, 102); auto handle = opened.String(); opened.End();
            std::vector<Entry> result;
            try {
                for (;;) {
                    Wire next; next.String(handle); auto response = Request(12, next, 104, true);
                    if (response.bytes.front() == 101) break;
                    const auto count = response.U32(); if (count > 10000 || result.size() + count > 10000) throw std::runtime_error("SFTP directory limit exceeded");
                    if (!count) throw std::runtime_error("Empty nonterminal SFTP directory response");
                    for (uint32_t i = 0; i < count; ++i) { auto name = response.String(); response.String(); auto a = ReadAttributes(response); if (name != "." && name != "..") { if (!ValidUtf8(name) || name.empty() || name.find_first_of("/\\") != name.npos || name.find('\0') != name.npos) throw std::runtime_error("Unsafe remote filename"); result.push_back({std::move(name), a}); } }
                    response.End();
                }
            } catch (...) { try { Close(handle); } catch (...) {} throw; }
            Close(handle); return result;
        }
        std::string ReadChunk(std::string_view handle, uint64_t offset)
        {
            Wire request; request.String(handle); request.U64(offset); request.U32(32768); auto response = Request(5, request, 103, true);
            if (response.bytes.front() == 101) return {};
            auto data = response.String(); response.End(); if (data.empty() || data.size() > 32768) throw std::runtime_error("Invalid SFTP data length"); return data;
        }
        void WriteChunk(std::string_view handle, uint64_t offset, std::string_view data)
        {
            if (data.empty() || data.size() > 32768) throw std::runtime_error("Invalid SFTP write length");
            Wire request; request.String(handle); request.U64(offset); request.String(data); auto response = Request(6, request, 101); CheckStatus(response);
        }
    private:
        Read _read; Write _write; uint32_t _id = 0; bool _atomicReplace = false;
        void Send(const Wire& body) { if (body.bytes.empty() || body.bytes.size() > MaxPacket) throw std::runtime_error("SFTP packet limit exceeded"); Wire header; header.U32(static_cast<uint32_t>(body.bytes.size())); _write(header.bytes); _write(body.bytes); }
        Wire Receive() { Wire header; header.bytes.resize(4); _read(header.bytes); const auto size = header.U32(); if (!size || size > MaxPacket) throw std::runtime_error("Invalid SFTP packet length"); Wire body; body.bytes.resize(size); _read(body.bytes); body.cursor = 1; return body; }
        static uint32_t Status(Wire& response) { const auto status = response.U32(); response.String(); response.String(); response.End(); return status; }
        static void CheckStatus(Wire& response) { const auto status = Status(response); if (status) throw StatusError(status); }
        Wire Request(uint8_t type, const Wire& fields, uint8_t expected, bool eof = false)
        {
            if (++_id == 0) throw std::runtime_error("SFTP request limit exceeded");
            Wire request; request.bytes.push_back(type); request.U32(_id); request.bytes.insert(request.bytes.end(), fields.bytes.begin(), fields.bytes.end()); Send(request);
            auto response = Receive(); if (response.U32() != _id) throw std::runtime_error("Mismatched SFTP request identity");
            if (response.bytes.front() == 101 && expected != 101) { const auto code = Status(response); if (eof && code == 1) return response; throw StatusError(code); }
            if (response.bytes.front() != expected) throw std::runtime_error("Unexpected SFTP response type");
            return response;
        }
    };
}
