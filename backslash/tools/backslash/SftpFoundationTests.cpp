// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#include "../../src/cascadia/inc/SftpProfile.h"
#include "../../src/cascadia/inc/SftpLocal.h"
#include <fstream>
#include <iostream>
using namespace Microsoft::Terminal::Backslash::Transfers::Sftp;
void Require(bool value){if(!value)throw std::runtime_error("Fixture assertion failed");}
template<typename F>void Reject(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}Require(rejected);}
int Run(int argc,wchar_t** argv)
{
    Require(argc==2);const auto root=std::filesystem::path(argv[1]);Require(!std::filesystem::exists(root));std::filesystem::create_directory(root);
    ProfileSnapshot profile{L"saved-id",L"Backslash.Terminal.SSH",LR"("%BACKSLASH_SSH_EXE%" "my-alias")"};
    const auto exe=LR"(C:\Windows\System32\OpenSSH\ssh.exe)";
    auto plan=ResolveProfile(profile,L"saved-id",exe,LR"(D:\portable\settings)");Require(plan.hostAlias&&plan.arguments.size()==1&&plan.arguments[0]==L"my-alias"&&plan.executable==exe);
    Reject([&]{ResolveProfile(profile,L"different",exe,L"");});profile.transient=true;Reject([&]{ResolveProfile(profile,L"saved-id",exe,L"");});profile.transient=false;
    profile.baseCommandline=LR"("C:\Windows\System32\OpenSSH\ssh.exe" host bash -l)";Reject([&]{ResolveProfile(profile,L"saved-id",exe,L"");});
    profile.baseCommandline=LR"(ssh.exe host)";Reject([&]{ResolveProfile(profile,L"saved-id",exe,L"");});
    profile.baseCommandline=LR"("%BACKSLASH_SSH_EXE%" -F none -o HostName=server -o User=user -o Port=22 -o "UserKnownHostsFile=%BACKSLASH_SETTINGS_DIR%/ssh_known_hosts" -o GlobalKnownHostsFile=none -o StrictHostKeyChecking=ask -o IdentityFile=none -o IdentityAgent=none -o IdentitiesOnly=yes alias)";
    plan=ResolveProfile(profile,L"saved-id",exe,LR"(D:\portable settings)");Require(!plan.hostAlias&&plan.arguments[8].find(L"%BACKSLASH_SETTINGS_DIR%")==std::wstring::npos);
    Reject([&]{ResolveProfile(profile,L"saved-id",exe,L"relative");});
    Require(ValidUtf8("\xc3\xa9")&&ValidUtf8("\xf0\x9f\x98\x80"));
    for(const auto bad:{std::string("a\0b",3),std::string("\xc0\xaf"),std::string("\xed\xa0\x80"),std::string("\xf4\x90\x80\x80"),std::string("\xe2\x82")}) {Require(!ValidUtf8(bad));Reject([&]{ValidatePath(bad);});}
    for(const auto* bad:{"..","CON","con.txt","a:b","a/b","a\\b","name.","name ","LPT1.txt"})Require(!SafeLocalName(bad));Require(SafeLocalName("space ; unicode-\xc3\xa9.txt"));
    {LocalTemporary locked(root/L"pending.txt");Require(!MoveFileExW(root.c_str(),(root.wstring()+L"-moved").c_str(),0));}
    const auto final=root/L"file.txt";
    {LocalTemporary file(final);file.Write("new data");file.Publish(false);}Require(std::filesystem::file_size(final)==8);
    {LocalTemporary file(final);file.Write("ignored");Reject([&]{file.Publish(false);});}Require(std::filesystem::file_size(final)==8);
    {LocalTemporary file(final);file.Write("replacement");file.Publish(true);}Require(std::filesystem::file_size(final)==11);
    {LocalTemporary file(root/L"FILE.TXT");file.Write("case collision");Reject([&]{file.Publish(false);});}Require(std::filesystem::file_size(final)==11);
    const auto link=root/L"linked";
    if(CreateSymbolicLinkW(link.c_str(),root.c_str(),SYMBOLIC_LINK_FLAG_DIRECTORY|SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE)) {
        Reject([&]{LocalTemporary file(link/L"through-link.txt");});
        Reject([&]{LocalTemporary file(link);file.Publish(true);});
    } else {std::cout<<"SKIP: owned directory symlink fixture unavailable (Windows "<<GetLastError()<<")\n";}
    {LocalTemporary file(final);LocalReplacement authority(final);file.Write("identity-bound");Require(!DeleteFileW(final.c_str()));file.PublishReplacing(authority);}Require(std::filesystem::file_size(final)==14);
    const auto retire=root/L"retire.txt";{std::ofstream f(retire);f<<"old-retire";}std::filesystem::path retiredBackup;
    {LocalTemporary file(retire);LocalReplacement target(retire);file.Write("new-retire");PublicationHooks hooks;hooks.beforeRetire=[]{throw std::runtime_error("Injected retirement failure");};try{file.PublishReplacing(target,hooks);Require(false);}catch(const LocalPublicationError& error){Require(error.published);retiredBackup=error.backup;}}
    Require(std::filesystem::exists(retiredBackup)&&std::filesystem::file_size(retire)==10);
    const auto restore=root/L"restore.txt";{std::ofstream f(restore);f<<"original-restore";}std::filesystem::path restoredBackup;
    {LocalTemporary file(restore);LocalReplacement target(restore);file.Write("new");PublicationHooks hooks;hooks.beforePublish=[&]{std::ofstream f(restore);f<<"collider";};try{file.PublishReplacing(target,hooks);Require(false);}catch(const LocalPublicationError& error){Require(!error.published);restoredBackup=error.backup;}}
    Require(std::filesystem::exists(restoredBackup)&&std::filesystem::file_size(restore)==8&&std::filesystem::file_size(restoredBackup)==16);
    std::filesystem::path abandoned;{LocalTemporary file(root/L"cancel.txt");abandoned=file.TemporaryPath();file.Write("partial");}Require(!std::filesystem::exists(abandoned)&&!std::filesystem::exists(root/L"cancel.txt"));
    std::filesystem::create_directory(root/L"directory");Reject([&]{LocalTemporary file(root/L"directory");file.Publish(true);});
    Reject([&]{LocalTemporary file(root/L"NUL.txt");});Reject([&]{LocalTemporary file(L"relative.txt");});
    std::cout<<"PASS: owned profile/alias/structured endpoint resolution, unsupported command rejection, strict UTF8/path boundaries, reserved-name checks, exclusive temporary publication/no-overwrite/replace/abandonment. Owned local fixtures only.\n"; return 0;
}
int wmain(int argc,wchar_t** argv){try{return Run(argc,argv);}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
