#pragma once
#include <string>
#include <string_view>
#include <stdexcept>
// Fixture only: bounded expected data, no output file reopening or retained extra bytes.
class FixtureDownloadVerifier {
 std::string expected;size_t received=0;bool bad=false,complete=false,begun=false;
public:
 static constexpr size_t Limit=65536;
 void Begin(std::string_view bytes){begun=false;complete=false;bad=true;expected.clear();received=0;if(bytes.empty()||bytes.size()>Limit)throw std::runtime_error("Fixture expected byte budget invalid");expected=bytes;received=0;bad=false;complete=false;begun=true;}
 void Observe(std::string_view bytes){if(!begun||complete||bad||received>expected.size()||bytes.size()>expected.size()-received||bytes!=std::string_view(expected).substr(received,bytes.size())){bad=true;throw std::runtime_error("Fixture download bytes mismatch/overflow");}received+=bytes.size();}
 void Published(){if(!begun||complete||bad||received!=expected.size())throw std::runtime_error("Fixture download incomplete/mismatched at publication");complete=true;}
 void RequirePublished()const {if(!complete||bad)throw std::runtime_error("Fixture remote write forbidden before marker verification");}
};


