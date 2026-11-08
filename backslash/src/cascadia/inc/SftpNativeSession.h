// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include "SftpService.h"
#include "SftpLocal.h"
namespace Microsoft::Terminal::Backslash::Transfers::Sftp
{
    struct LocalAuthority : Replacement
    {
        static const void* Key() noexcept {static const char key=0;return &key;}
        void* Query(const void* key)noexcept override{return key==Key()?this:nullptr;}
        std::shared_ptr<LocalTemporary> temporary;
        std::shared_ptr<LocalReplacement> destination;
    };
    class NativeSession final : public Session
    {
    public:
        #ifdef BACKSLASH_SFTP_FIXTURE_DIAGNOSTICS
        explicit NativeSession(const ConnectionPlan& plan,std::function<void(std::string_view)> diagnostics={},std::function<void(std::string_view)> downloadObserver={}) : _transport(plan.executable,plan.arguments,30000,std::move(diagnostics)),_client([this](auto bytes){_transport.Read(bytes);},[this](auto bytes){_transport.Write(bytes);}),_fixtureDownloadObserver(std::move(downloadObserver)) {}
#else
        explicit NativeSession(const ConnectionPlan& plan) : _transport(plan.executable,plan.arguments),_client([this](auto bytes){_transport.Read(bytes);},[this](auto bytes){_transport.Write(bytes);}) {}
#endif
        std::vector<Entry> Browse(std::string_view path)override{Initialize();return _client.List(path);}
        void Prepare(const Request& request,const std::shared_ptr<Replacement>& authority)override
        {
            if(request.direction==Direction::Download) {
                if(authority) {
                    auto* local=static_cast<LocalAuthority*>(authority->Query(LocalAuthority::Key()));
                    if(local)_authority=std::shared_ptr<LocalAuthority>(authority,local);
                    if(!_authority||_wcsicmp(_authority->destination->Path().c_str(),request.local.c_str()))throw std::runtime_error("Destination authority does not match this transfer");
                    _temporary=_authority->temporary;
                } else {
                    _temporary=std::make_shared<LocalTemporary>(request.local);
                    const auto attributes=GetFileAttributesW(request.local.c_str());
                    if(attributes!=INVALID_FILE_ATTRIBUTES) {
                        auto token=std::make_shared<LocalAuthority>();token->temporary=_temporary;token->destination=std::make_shared<LocalReplacement>(request.local);throw Conflict(token);
                    }
                    if(GetLastError()!=ERROR_FILE_NOT_FOUND)throw std::runtime_error("Cannot inspect local destination");
                }
            } else {
                if(authority)throw std::runtime_error("Remote identity-bound replacement is unavailable; choose a new filename");
                const std::filesystem::path local{request.local};
                _sourceParents=std::make_unique<DirectoryLocks>(local.parent_path());
                if(local.filename().native().find(L':' )!=std::wstring::npos)throw std::runtime_error("Alternate data stream uploads are unsupported");
                _source.Reset(CreateFileW(request.local.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr));WinCheck(_source.value!=INVALID_HANDLE_VALUE);
                FILE_ATTRIBUTE_TAG_INFO info{};WinCheck(GetFileInformationByHandleEx(_source.value,FileAttributeTagInfo,&info,sizeof(info)));
                if(info.FileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)||GetFileType(_source.value)!=FILE_TYPE_DISK)throw std::runtime_error("Upload requires a local regular file");
                LARGE_INTEGER size{};WinCheck(GetFileSizeEx(_source.value,&size));_size=static_cast<uint64_t>(size.QuadPart);
                Initialize();
                try{_client.Stat(request.remote);throw std::runtime_error("Remote destination exists; choose a new filename (safe identity-bound remote replacement is unavailable)");}
                catch(const StatusError& error){if(error.code!=2)throw;}
                const auto separator=request.remote.rfind('/');const auto directory=separator==request.remote.npos?std::string{}:request.remote.substr(0,separator+1);
                GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Cannot create remote temporary identity");
                wchar_t guid[40];StringFromGUID2(id,guid,40);std::string ascii;for(auto c:std::wstring_view(guid))ascii+=static_cast<char>(c);_remoteTemporary=directory+".backslash-"+ascii+".partial";
            }
        }
        void Transfer(const Request& request,const ProgressCallback& progress,const CancelCheck& cancel)override
        {
            Initialize();
            if(request.direction==Direction::Download)Download(_client,request.remote,[this](auto bytes){_temporary->Write(bytes);
#ifdef BACKSLASH_SFTP_FIXTURE_DIAGNOSTICS
                if(_fixtureDownloadObserver)_fixtureDownloadObserver(bytes);
#endif
            },progress,cancel);
            else {
                _remoteMayRemain=true;
                try{Upload(_client,_remoteTemporary,_size,[this]{std::string bytes(32768,0);DWORD read=0;WinCheck(ReadFile(_source.value,bytes.data(),static_cast<DWORD>(bytes.size()),&read,nullptr));bytes.resize(read);return bytes;},progress,cancel);}
                catch(const std::exception& error){throw std::runtime_error(std::string(error.what())+"; remote temporary may remain: "+_remoteTemporary);}
            }
        }
        void Publish(const Request& request,const std::shared_ptr<Replacement>&)override
        {
            if(request.direction==Direction::Download) {if(_authority){try{_temporary->PublishReplacing(*_authority->destination);}catch(const LocalPublicationError& error){_retained.push_back({error.backup.wstring(),{},error.what()});if(error.published)throw PublishedWithCleanup(error.what(),_retained);throw;}}else _temporary->Publish(false);}
            else {
                try{_client.Rename(_remoteTemporary,request.remote,false);_remoteMayRemain=false;}
                catch(const std::exception& error){throw std::runtime_error(std::string(error.what())+"; remote temporary may remain: "+_remoteTemporary);}
            }
        }
        std::vector<RetainedArtifact> Retained()const override{auto files=_retained;if(_remoteMayRemain)files.push_back({{},_remoteTemporary,"Remote temporary may remain; cleanup not confirmed"});return files;}
        std::vector<RetainedArtifact> ShutdownContext()const override
        {
            auto files=Retained();if(_temporary)files.push_back({_temporary->TemporaryPath().wstring(),{},"Local temporary may remain if forced to quit"});if(_authority)files.push_back({_authority->destination->Backup().wstring(),{},"Original-file backup may remain if forced to quit"});return files;
        }
        void Cancel()noexcept override{_transport.Cancel();}
    private:
        ProcessTransport _transport;Client _client;
#ifdef BACKSLASH_SFTP_FIXTURE_DIAGNOSTICS
        std::function<void(std::string_view)> _fixtureDownloadObserver;
#endif
        bool _initialized=false,_remoteMayRemain=false;
        std::unique_ptr<DirectoryLocks> _sourceParents;std::vector<RetainedArtifact> _retained;Handle _source;uint64_t _size=0;std::string _remoteTemporary;
        std::shared_ptr<LocalTemporary> _temporary;std::shared_ptr<LocalAuthority> _authority;
        void Initialize(){if(!_initialized){_client.Initialize();_initialized=true;}}
    };
    inline Service& SharedService()
    {
        static Service service([](const ConnectionPlan& connection){return std::make_shared<NativeSession>(connection);});return service;
    }
}
