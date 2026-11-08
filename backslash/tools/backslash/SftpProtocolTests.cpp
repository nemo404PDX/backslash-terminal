// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "../../src/cascadia/inc/SftpProtocol.h"
#include "../../src/cascadia/inc/SftpProcess.h"
#include "../../src/cascadia/inc/SftpTransfer.h"
#include <algorithm>
#include <iostream>
using namespace Microsoft::Terminal::Backslash::Transfers::Sftp;
void Require(bool value) { if (!value) throw std::runtime_error("Assertion failed"); }
template<typename F> void Reject(F action) { bool rejected = false; try { action(); } catch (const std::runtime_error&) { rejected = true; } Require(rejected); }
struct Peer {
    Bytes input, output; size_t offset = 0;
    Client ClientFor() { return Client([this](std::span<uint8_t> target) { if (target.size() > input.size() - offset) throw std::runtime_error("Disconnected"); std::copy_n(input.begin() + offset, target.size(), target.begin()); offset += target.size(); }, [this](std::span<const uint8_t> source) { output.insert(output.end(), source.begin(), source.end()); }); }
    void Reply(uint8_t type, uint32_t id, Wire fields = {}) { Wire body; body.bytes.push_back(type); if (type != 2) body.U32(id); body.bytes.insert(body.bytes.end(), fields.bytes.begin(), fields.bytes.end()); Wire header; header.U32(static_cast<uint32_t>(body.bytes.size())); input.insert(input.end(), header.bytes.begin(), header.bytes.end()); input.insert(input.end(), body.bytes.begin(), body.bytes.end()); }
    void StatusReply(uint32_t id, uint32_t code) { Wire w; w.U32(code); w.String("server text deliberately not logged"); w.String(""); Reply(101, id, w); }
};
int main() {
    Require(QuoteArgument(LR"(C:\space dir\)") == LR"("C:\space dir\\")");
    for(const auto& [path,parent]:std::vector<std::pair<std::string,std::string>>{{".",".."},{"..","../.."},{"../..","../../.."},{"namedrelative","."},{"namedrelative/","."},{"folder/child","folder"},{"/","/"},{"////","/"},{"/folder/","/"},{"/folder/child///","/folder"},{"folder/../",".."}})Require(ParentPath(path)==parent);
    Wire numbers; numbers.U64(0x0123456789abcdef); numbers.String(std::string("a\0b",3)); Require(numbers.U64() == 0x0123456789abcdef); Require(numbers.String() == std::string("a\0b",3)); numbers.End(); Reject([&]{numbers.U32();});
    Peer p; Wire version; version.U32(3); p.Reply(2,0,version); Wire handle; handle.String("opaque-handle"); p.Reply(102,1,handle); Wire data; data.String(std::string("a\0b",3)); p.Reply(103,2,data); p.StatusReply(3,1); p.StatusReply(4,0);
    auto client = p.ClientFor(); client.Initialize(); auto h = client.Open("/space ; unicode",false,false); Require(h == "opaque-handle"); Require(client.ReadChunk(h,0) == std::string("a\0b",3)); Require(client.ReadChunk(h,3).empty()); client.Close(h);
    Peer listing; listing.Reply(102,1,handle); Wire names; names.U32(1); names.String("file.txt"); names.String("ignored presentation"); names.U32(5); names.U64(42); names.U32(0100644); listing.Reply(104,2,names); listing.StatusReply(3,1); listing.StatusReply(4,0); auto l = listing.ClientFor(); auto entries = l.List("."); Require(entries.size()==1 && entries[0].attributes.Regular() && entries[0].attributes.size==42);
    Peer denied; denied.StatusReply(1,3); auto d = denied.ClientFor(); Reject([&]{d.Open("/denied",false,false);});
    Peer mismatch; mismatch.Reply(102,99,handle); auto m = mismatch.ClientFor(); Reject([&]{m.Open("/file",false,false);});
    Peer huge; huge.input={0,16,0,1}; auto big = huge.ClientFor(); Reject([&]{big.Open("/file",false,false);});
    Peer malformed; malformed.Reply(102,1,{}); auto bad = malformed.ClientFor(); Reject([&]{bad.Open("/file",false,false);});
    Peer upload; upload.Reply(102,1,handle); upload.StatusReply(2,0); upload.StatusReply(3,0); auto u = upload.ClientFor(); auto uh=u.Open("/new",true,false); u.WriteChunk(uh,0,"payload"); u.Close(uh);
    Peer transfer; Wire attrs; attrs.U32(5); attrs.U64(3); attrs.U32(0100644); transfer.Reply(105,1,attrs); transfer.Reply(102,2,handle); transfer.Reply(103,3,data); transfer.StatusReply(4,1); transfer.StatusReply(5,0);
    auto session=transfer.ClientFor(); std::string downloaded; uint64_t observed=0;
    Download(session,"/regular",[&](std::string_view bytes){downloaded+=bytes;},[&](Progress value){observed=value.completed;},[]{});
    Require(downloaded==std::string("a\0b",3) && observed==3);
    Peer canceled; auto cs=canceled.ClientFor(); Reject([&]{Download(cs,"/never-opened",[](auto){},[](auto){},[]{throw std::runtime_error("Canceled");});}); Require(canceled.output.empty());
    Peer replacement; auto rs=replacement.ClientFor(); Reject([&]{rs.Rename("/temp","/final",true);}); Require(replacement.output.empty());
    Wire unknown; unknown.U32(16); Reject([&]{ReadAttributes(unknown);});
    std::cout << "PASS: initialization, binary reads, EOF, directory attributes, upload acknowledgments, denial, mismatched IDs, oversized/truncated frames and unsupported attributes. Fake byte streams only.\n";
}
