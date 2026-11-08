// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include <Windows.h>
#include <objbase.h>
#include <filesystem>
#include "SftpProcess.h"
namespace Microsoft::Terminal::Backslash::Transfers::Sftp
{
    class DirectoryLocks
    {
    public:
        explicit DirectoryLocks(const std::filesystem::path& directory)
        {
            const auto path=directory.lexically_normal();
            if(path.native().find_first_of(std::wstring_view{L"\0\r\n",3})!=std::wstring::npos||!path.is_absolute()||path.native().starts_with(L"\\\\")||path.native().find(L':',2)!=std::wstring::npos)throw std::runtime_error("Choose an absolute local drive directory");
            const auto drive=GetDriveTypeW(path.root_path().c_str());if(drive!=DRIVE_FIXED&&drive!=DRIVE_REMOVABLE&&drive!=DRIVE_RAMDISK)throw std::runtime_error("SFTP preview requires a local fixed/removable drive");
            auto cursor=path.root_path();Add(cursor);for(const auto& component:path.relative_path()){cursor/=component;Add(cursor);}
        }
    private:
        std::vector<std::unique_ptr<Handle>> _handles;
        void Add(const std::filesystem::path& path)
        {
            auto handle=std::make_unique<Handle>(CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr));WinCheck(handle->value!=INVALID_HANDLE_VALUE);
            FILE_ATTRIBUTE_TAG_INFO info{};WinCheck(GetFileInformationByHandleEx(handle->value,FileAttributeTagInfo,&info,sizeof(info)));
            if(!(info.FileAttributes&FILE_ATTRIBUTE_DIRECTORY)||(info.FileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Local directory is unavailable or passes through a reparse path");_handles.emplace_back(std::move(handle));
        }
    };
    inline void RenameOwnedHandle(HANDLE file,const std::filesystem::path& path,bool replace=false)
    {
        const auto name=path.wstring();const auto size=offsetof(FILE_RENAME_INFO,FileName)+(name.size()+1)*sizeof(wchar_t);
        std::vector<uint8_t> buffer(size);auto* rename=reinterpret_cast<FILE_RENAME_INFO*>(buffer.data());
        rename->ReplaceIfExists=replace?TRUE:FALSE;rename->RootDirectory=nullptr;rename->FileNameLength=static_cast<DWORD>(name.size()*sizeof(wchar_t));
        memcpy(rename->FileName,name.data(),name.size()*sizeof(wchar_t));WinCheck(SetFileInformationByHandle(file,FileRenameInfo,rename,static_cast<DWORD>(size)));
    }
    // A replacement decision owns this exact destination handle; no writable or
    // delete-sharing handles may appear while approval/transfer is pending.
    class LocalReplacement
    {
    public:
        explicit LocalReplacement(std::filesystem::path path) : _path(std::move(path))
        {
            _file.Reset(CreateFileW(_path.c_str(),GENERIC_READ|DELETE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));WinCheck(_file.value!=INVALID_HANDLE_VALUE);
            FILE_ATTRIBUTE_TAG_INFO info{};WinCheck(GetFileInformationByHandleEx(_file.value,FileAttributeTagInfo,&info,sizeof(info)));
            if(info.FileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Destination is not a replaceable regular file");
            GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Cannot create backup identity");wchar_t text[40];StringFromGUID2(id,text,40);
            _backup=_path.parent_path()/(std::wstring(L".backslash-backup-")+text);
        }
        void MoveAside(){if(_used)throw std::runtime_error("Replacement authority already consumed");_used=true;RenameOwnedHandle(_file.value,_backup);_moved=true;}
        bool Restore()noexcept {if(!_moved)return true;try{RenameOwnedHandle(_file.value,_path);_moved=false;return true;}catch(...){return false;}}
        void Retire(){FILE_DISPOSITION_INFO disposition{TRUE};WinCheck(SetFileInformationByHandle(_file.value,FileDispositionInfo,&disposition,sizeof(disposition)));_moved=false;}
        ~LocalReplacement(){Restore();}
        const std::filesystem::path& Path()const{return _path;}
        const std::filesystem::path& Backup()const{return _backup;}
    private:
        Handle _file;std::filesystem::path _path,_backup;bool _moved=false,_used=false;
    };
    struct LocalPublicationError : std::runtime_error
    {
        bool published;std::filesystem::path backup;
        LocalPublicationError(bool done,std::filesystem::path retained) : std::runtime_error(done?"Published new file; original destination backup retained":"Publication conflict; original destination retained in backup"),published(done),backup(std::move(retained)){}
    };
    struct PublicationHooks {std::function<void()> beforePublish,beforeRetire;}; // Deterministic owned-fixture fault injection only.
    class LocalTemporary
    {
    public:
        explicit LocalTemporary(const std::filesystem::path& finalPath) : _final(finalPath.lexically_normal())
        {
            if(!_final.is_absolute()||_final.native().starts_with(L"\\\\")||_final.filename().empty()||_final.native().find(L':',2)!=std::wstring::npos)throw std::runtime_error("Choose an absolute local drive file path");
            const auto leaf=_final.filename().wstring();
            const auto length=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,leaf.data(),static_cast<int>(leaf.size()),nullptr,0,nullptr,nullptr);
            if(!length)throw std::runtime_error("Invalid local filename Unicode");
            std::string utf8(static_cast<size_t>(length),0);WinCheck(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,leaf.data(),static_cast<int>(leaf.size()),utf8.data(),length,nullptr,nullptr)!=0);
            if(!SafeLocalName(utf8))throw std::runtime_error("Unsupported local filename");
            if(leaf==L"."||leaf==L".."||leaf.back()==L'.'||leaf.back()==L' ')throw std::runtime_error("Unsupported local filename");
            // Hold each directory without delete sharing for the entire operation.
            // This prevents an ordinary concurrent rename/reparse substitution.
            _guard=std::make_unique<DirectoryLocks>(_final.parent_path());
            GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Cannot create temporary identity");
            wchar_t text[40];if(!StringFromGUID2(id,text,40))throw std::runtime_error("Cannot format temporary identity");
            _temporary=_final.parent_path()/(std::wstring(L".backslash-")+text+L".partial");
            _file.Reset(CreateFileW(_temporary.c_str(),GENERIC_WRITE|DELETE|FILE_READ_ATTRIBUTES,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
            WinCheck(_file.value!=INVALID_HANDLE_VALUE);
        }
        ~LocalTemporary() { if(!_published&&_file.value&&_file.value!=INVALID_HANDLE_VALUE) {FILE_DISPOSITION_INFO disposition{TRUE};SetFileInformationByHandle(_file.value,FileDispositionInfo,&disposition,sizeof(disposition));} }
        void Write(std::string_view bytes)
        {
            if(_published)throw std::runtime_error("Temporary destination already published");
            while(!bytes.empty()) {DWORD count=0;WinCheck(WriteFile(_file.value,bytes.data(),static_cast<DWORD>(std::min<size_t>(bytes.size(),32768)),&count,nullptr));if(!count)throw std::runtime_error("Local destination made no progress");bytes.remove_prefix(count);}
        }
        void Publish(bool explicitlyReplace)
        {
            if(_published)throw std::runtime_error("Temporary destination already published");
            WinCheck(FlushFileBuffers(_file.value));
            const auto attributes=GetFileAttributesW(_final.c_str());
            if(attributes!=INVALID_FILE_ATTRIBUTES) {
                if(!explicitlyReplace)throw std::runtime_error("Destination exists; explicit replacement or a new name is required");
                if(attributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Cannot replace a directory or reparse destination");
            } else if(GetLastError()!=ERROR_FILE_NOT_FOUND)throw std::runtime_error("Cannot inspect local destination");
            RenameOwnedHandle(_file.value,_final,explicitlyReplace);
            _published=true;
        }
        void PublishReplacing(LocalReplacement& target,const PublicationHooks& hooks={})
        {
            if(_wcsicmp(_final.c_str(),target.Path().c_str()))throw std::runtime_error("Replacement target changed");
            target.MoveAside();
            try{if(hooks.beforePublish)hooks.beforePublish();Publish(false);}catch(...){if(!target.Restore())throw LocalPublicationError(false,target.Backup());throw;}
            // Publication already succeeded: a failed backup retirement is a partial
            // cleanup error, not a claim the new file did not reach its destination.
            try{if(hooks.beforeRetire)hooks.beforeRetire();target.Retire();}catch(...){throw LocalPublicationError(true,target.Backup());}
        }
        std::filesystem::path TemporaryPath() const { return _temporary; }
    private:
        std::filesystem::path _final,_temporary;std::unique_ptr<DirectoryLocks> _guard;Handle _file;bool _published=false;
    };
}
