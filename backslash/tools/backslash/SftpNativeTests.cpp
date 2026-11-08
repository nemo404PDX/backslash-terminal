// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "../../src/cascadia/inc/SftpNativeSession.h"
#include <map>
#include <fstream>
#include <iostream>
using namespace Microsoft::Terminal::Backslash::Transfers::Sftp;
void Require(bool value){if(!value)throw std::runtime_error("Native session fixture assertion failed");}
void Exact(HANDLE pipe,std::span<uint8_t> bytes){while(!bytes.empty()){DWORD n=0;if(!ReadFile(pipe,bytes.data(),static_cast<DWORD>(bytes.size()),&n,nullptr)||!n)throw std::runtime_error("Fixture pipe closed");bytes=bytes.subspan(n);}}
void Send(Wire body){Wire length;length.U32(static_cast<uint32_t>(body.bytes.size()));DWORD n=0;WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),length.bytes.data(),4,&n,nullptr);WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),body.bytes.data(),static_cast<DWORD>(body.bytes.size()),&n,nullptr);}
void Status(uint32_t id,uint32_t code){Wire out;out.bytes.push_back(101);out.U32(id);out.U32(code);out.String("");out.String("");Send(out);}
void Attr(Wire& out,uint64_t size){out.U32(5);out.U64(size);out.U32(0100644);}
int Server(bool stall,bool dropRenameAck)
{
    std::map<std::string,std::string> files{{"/source",std::string("a\0b\xff",4)}};
    for(;;){Wire input;input.bytes.resize(4);Exact(GetStdHandle(STD_INPUT_HANDLE),input.bytes);const auto length=input.U32();if(length>MaxPacket||!length)return 5;input.bytes.resize(length);input.cursor=1;Exact(GetStdHandle(STD_INPUT_HANDLE),input.bytes);const auto type=input.bytes[0];
        if(type==1){Require(input.U32()==3);Wire out;out.bytes.push_back(2);out.U32(3);Send(out);continue;}
        const auto id=input.U32();
        if(type==7){const auto path=input.String();if(!files.contains(path)){Status(id,2);continue;}Wire out;out.bytes.push_back(105);out.U32(id);Attr(out,files[path].size());Send(out);}
        else if(type==3){auto path=input.String();const auto flags=input.U32();input.U32();if((flags&32)&&files.contains(path)){Status(id,4);continue;}if(flags&8)files[path]="";if(!files.contains(path)){Status(id,2);continue;}Wire out;out.bytes.push_back(102);out.U32(id);out.String(path);Send(out);}
        else if(type==5){auto path=input.String();const auto offset=input.U64();const auto count=input.U32();if(offset>=files[path].size()){Status(id,1);continue;}Wire out;out.bytes.push_back(103);out.U32(id);out.String(std::string_view(files[path]).substr(static_cast<size_t>(offset),count));Send(out);}
        else if(type==6){auto path=input.String();const auto offset=input.U64();auto data=input.String();if(offset>1024*1024||data.size()>32768){Status(id,4);continue;}auto& file=files[path];file.resize((std::max)(file.size(),static_cast<size_t>(offset)+data.size()));file.replace(static_cast<size_t>(offset),data.size(),data);Status(id,0);if(stall)Sleep(INFINITE);}
        else if(type==4){input.String();Status(id,0);}
        else if(type==18){auto source=input.String();auto destination=input.String();if(files.contains(destination)||files[source]!="upload-bytes"){Status(id,4);continue;}files[destination]=files[source];files.erase(source);if(dropRenameAck)return 0;Status(id,0);}
        else if(type==11){input.String();Wire out;out.bytes.push_back(102);out.U32(id);out.String("directory");Send(out);}
        else if(type==12){static bool listed=false;if(listed){Status(id,1);continue;}listed=true;Wire out;out.bytes.push_back(104);out.U32(id);out.U32(1);out.String("source");out.String("ignored");Attr(out,4);Send(out);}
        else{Status(id,8);}
    }
}
void Wait(Service& service){const auto start=GetTickCount64();while(service.Busy()){Require(GetTickCount64()-start<5000);Sleep(1);}}
int Run(const std::filesystem::path& root)
{
    Require(!std::filesystem::exists(root));std::filesystem::create_directory(root);wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);ConnectionPlan plan{exe,{L"--fake-sftp"},true};
    Service service([](const auto& connection){return std::make_shared<NativeSession>(connection);});service.Enabled(true);
    service.Browse(plan,"/");Wait(service);Require(service.Browser().error.empty()&&service.Browser().entries.size()==1);
    Request download{plan,Direction::Download,(root/L"download.bin").wstring(),"/source"};service.Submit(download);Wait(service);Require(service.Jobs().back().phase==JobPhase::Completed);std::ifstream read(root/L"download.bin",std::ios::binary);std::string bytes{std::istreambuf_iterator<char>(read),{}};Require(bytes==std::string("a\0b\xff",4));read.close();
    const auto id=service.Submit(download);Wait(service);auto pending=service.Jobs().back();Require(pending.phase==JobPhase::AwaitingDecision);Require(!DeleteFileW(download.local.c_str()));Require(service.Replace(id,pending.decision));Wait(service);Require(service.Jobs().back().phase==JobPhase::Completed);
    {std::ofstream file(root/L"upload.bin",std::ios::binary);file<<"upload-bytes";}
    Request upload{plan,Direction::Upload,(root/L"upload.bin").wstring(),"/destination"};service.Submit(upload);Wait(service);Require(service.Jobs().back().phase==JobPhase::Completed);
    upload.remote="/source";service.Submit(upload);Wait(service);Require(service.Jobs().back().phase==JobPhase::Failed&&service.Jobs().back().error.find("choose a new filename")!=std::string::npos);
    {std::ofstream file(root/L"large.bin",std::ios::binary);file<<std::string(100000,'x');}
    Request partial{plan,Direction::Upload,(root/L"large.bin").wstring(),"/partial"};partial.connection.arguments={L"--stall-after-write",L"--fake-sftp"};const auto canceled=service.Submit(partial);const auto began=GetTickCount64();while(service.Jobs().back().progress.completed==0){Require(GetTickCount64()-began<5000);Sleep(1);}Require(service.Cancel(canceled));Wait(service);const auto result=service.Jobs().back();Require(result.phase==JobPhase::Canceled&&!result.published&&result.retained.size()==1&&result.retained[0].remote.starts_with("/.backslash-")&&result.retained[0].remote.ends_with(".partial"));
    Request lostAck{plan,Direction::Upload,(root/L"upload.bin").wstring(),"/lost-ack"};lostAck.connection.arguments={L"--drop-rename-ack",L"--fake-sftp"};const auto uncertain=service.Submit(lostAck);Wait(service);const auto unknown=service.Jobs().back();Require(unknown.phase==JobPhase::Failed&&!unknown.published&&unknown.publicationUncertain&&unknown.error.find("unconfirmed")!=std::string::npos);bool retryBlocked=false;try{service.Retry(uncertain);}catch(const std::exception&){retryBlocked=true;}Require(retryBlocked);
    std::cout<<"PASS: real process/pipes/protocol/native adapter/service browse, binary download, exact local replacement, upload and non-destructive remote conflict. Fake child only, no network.\n";return 0;
}
int wmain(int argc,wchar_t** argv){try{bool fake=false,stall=false,dropRenameAck=false;for(int i=1;i<argc;++i){if(std::wstring_view(argv[i])==L"--fake-sftp")fake=true;if(std::wstring_view(argv[i])==L"--stall-after-write")stall=true;if(std::wstring_view(argv[i])==L"--drop-rename-ack")dropRenameAck=true;}if(fake)return Server(stall,dropRenameAck);Require(argc==2);return Run(argv[1]);}catch(const std::exception& error){std::cerr<<error.what()<<std::endl;return 1;}}
