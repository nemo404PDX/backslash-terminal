// Copyright (c) Backslash Terminal contributors.
// Licensed under the MIT license.
#pragma once
#include "SftpWorkspaceView.h"
#include "SftpService.h"
#include "SftpLocal.h"
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Xaml.Input.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Microsoft.Terminal.Settings.Model.h>
namespace Microsoft::Terminal::Backslash::Transfers::Sftp
{
    namespace Model=winrt::Microsoft::Terminal::Settings::Model;
    using namespace winrt::Windows::UI::Xaml;
    using namespace winrt::Windows::UI::Xaml::Controls;
    using winrt::hstring;
    using winrt::box_value;
    class WorkspaceController : public std::enable_shared_from_this<WorkspaceController>
    {
    public:
        static std::shared_ptr<WorkspaceController> Create(const Grid& root,Service& service,std::function<void()> changed={})
        {
            auto self=std::shared_ptr<WorkspaceController>(new WorkspaceController(root,service,std::move(changed)));self->_BuildSftp();
            const auto dispatcher=root.Dispatcher();self->_subscription=service.Subscribe([weak=std::weak_ptr(self),dispatcher]{try{dispatcher.RunAsync(winrt::Windows::UI::Core::CoreDispatcherPriority::Normal,[weak]{if(const auto owner=weak.lock();owner&&!owner->_closed)owner->_RenderSftp();});}catch(...){}});return self;
        }
        ~WorkspaceController(){Close();}
        void Close(){if(_closed)return;_closed=true;++_localGeneration;_service.Unsubscribe(_subscription);}
        void UpdateSettings(const Model::CascadiaSettings& settings);
        void Error(const hstring& text){if(!_closed){_SftpError(text);const auto message=_root.FindName(L"TransferActionError").as<TextBlock>();message.Text(text);message.Visibility(text.empty()?Visibility::Collapsed:Visibility::Visible);}}
        void ConfirmReplacement(uint64_t job,uint64_t decision){if(!_closed){Error({});_ReplaceSftp(job,decision);}}
        WorkspaceView View()const{return _view;}
    private:
        WorkspaceController(Grid root,Service& service,std::function<void()> changed) : _root(std::move(root)),_service(service),_changed(std::move(changed)){}
        void _Refresh(){if(!_closed&&_changed)_changed();}
        void _BuildSftp();void _RenderSftp();void _BrowseRemote();winrt::fire_and_forget _BrowseLocal();winrt::fire_and_forget _ReplaceSftp(uint64_t job,uint64_t decision);void _SubmitSftp(bool upload);void _SftpError(const hstring& message);
        Grid _root{nullptr};Service& _service;std::function<void()> _changed;WorkspaceView _view;
        TextBox _localPath{nullptr},_remotePath{nullptr},_transferName{nullptr};TextBlock _connectionName{nullptr},_sftpStatus{nullptr};ListView _localFiles{nullptr},_remoteFiles{nullptr};Grid _browserGrid{nullptr};
        std::optional<ConnectionPlan> _sftpConnection;std::vector<std::filesystem::path> _localEntries;std::vector<bool> _localDirectories;std::vector<Entry> _remoteEntries;std::wstring _localDirectory;std::string _remoteDirectory;
        uint64_t _subscription=0,_localGeneration=0,_renderedRemoteGeneration=0;bool _localBusy=false,_closed=false;
    };
    inline void WorkspaceController::UpdateSettings(const Model::CascadiaSettings& settings)
    {
        _root.RequestedTheme(settings.GlobalSettings().CurrentTheme().RequestedTheme());
        const auto previousConnection=_sftpConnection;
        _sftpConnection.reset();_connectionName.Text(L"SFTP");_connectionName.Visibility(Visibility::Collapsed);
        try {
            const auto config=winrt::Windows::Data::Json::JsonObject::Parse(settings.GlobalSettings().TransferBackendsJson());
            if(config.Size()!=5||config.GetNamedNumber(L"version")!=1)throw std::runtime_error("Unsupported Transfers backend settings");
            const auto sftp=config.GetNamedObject(L"sftp");if(sftp.Size()!=2)throw std::runtime_error("Invalid SFTP settings");
            _service.Enabled(sftp.GetNamedBoolean(L"enabled"));
            if(!sftp.GetNamedBoolean(L"enabled"))throw std::runtime_error("Enable SFTP and choose a saved SSH profile in Transfers settings");
            const auto selected=sftp.GetNamedString(L"profile");if(selected.empty())throw std::runtime_error("Choose a saved SSH profile in Transfers settings");
            bool found=false;
            for(const auto& profile:settings.AllProfiles())if(profile.Guid()==winrt::guid{selected}) {
                wchar_t executable[32768]{};auto count=GetEnvironmentVariableW(L"BACKSLASH_SSH_EXE",executable,32768);
                if(!count||count>=32768){GetSystemDirectoryW(executable,32768);wcscat_s(executable,L"\\OpenSSH\\ssh.exe");}
                const auto base=profile.SshBaseCommandline().empty()?profile.Commandline():profile.SshBaseCommandline();
                const auto canonical=winrt::to_hstring(profile.Guid());
                ProfileSnapshot snapshot{std::wstring(canonical),std::wstring(profile.Source()),std::wstring(base),profile.Deleted(),profile.IsTransientHostSsh()};
                _sftpConnection=ResolveProfile(snapshot,std::wstring(canonical),executable,std::wstring(Model::CascadiaSettings::SettingsDirectory()));
                if(GetFileAttributesW(_sftpConnection->executable.c_str())==INVALID_FILE_ATTRIBUTES)throw std::runtime_error("OpenSSH ssh.exe is unavailable");
                _connectionName.Text(profile.Name());_connectionName.Visibility(Visibility::Visible);found=true;break;
            }
            if(!found)throw std::runtime_error("Choose an available saved SSH profile in Transfers settings");
            _sftpStatus.Text(L"Ready to connect. Existing key/agent authentication and a trusted host key are required.");
        }catch(const std::exception& error){_sftpConnection.reset();_service.Enabled(false);_SftpError(winrt::to_hstring(error.what()));}catch(...){_sftpConnection.reset();_service.Enabled(false);_SftpError(L"Invalid SFTP configuration");}
        if(previousConnection!=_sftpConnection){_remoteEntries.clear();_remoteFiles.Items().Clear();_remoteDirectory.clear();_renderedRemoteGeneration=0;}
        _RenderSftp();
    }
    inline void WorkspaceController::_SftpError(const hstring& message)
    {
        if(_sftpStatus){_sftpStatus.Text(message);}
    }
    inline void WorkspaceController::_BuildSftp()
    {
        auto view=BuildWorkspace(_root.FindName(L"SftpWorkspace").as<Grid>(),Model::CascadiaSettings::SettingsDirectory());
        _view=view;
        _connectionName=view.connection;_browserGrid=view.browsers;_localPath=view.localPath;_remotePath=view.remotePath;_localFiles=view.localFiles;_remoteFiles=view.remoteFiles;_transferName=view.filename;_sftpStatus=view.status;
        view.localGo.Click([weak=weak_from_this()](auto&&,auto&&){if(const auto self=weak.lock())self->_BrowseLocal();});
        view.remoteGo.Click([weak=weak_from_this()](auto&&,auto&&){if(const auto self=weak.lock())self->_BrowseRemote();});
        view.localUp.Click([weak=weak_from_this()](auto&&,auto&&){if(const auto self=weak.lock()){self->_localPath.Text(std::filesystem::path(std::wstring(self->_localPath.Text())).parent_path().wstring());self->_BrowseLocal();}});
        view.remoteUp.Click([weak=weak_from_this()](auto&&,auto&&){if(const auto self=weak.lock()){auto path=winrt::to_string(self->_remotePath.Text());path=ParentPath(path);self->_remotePath.Text(winrt::to_hstring(path));self->_BrowseRemote();}});
        _localFiles.DoubleTapped([weak=weak_from_this()](auto&&,auto&&){if(const auto self=weak.lock()){const auto index=self->_localFiles.SelectedIndex();if(index>=0&&static_cast<size_t>(index)<self->_localEntries.size()&&self->_localDirectories[index]){self->_localPath.Text(self->_localEntries[index].wstring());self->_BrowseLocal();}}});
        _remoteFiles.DoubleTapped([weak=weak_from_this()](auto&&,auto&&){if(const auto self=weak.lock()){const auto index=self->_remoteFiles.SelectedIndex();if(index>=0&&static_cast<size_t>(index)<self->_remoteEntries.size()&&self->_remoteEntries[index].attributes.Directory()){self->_remotePath.Text(winrt::to_hstring(self->_remoteDirectory+"/"+self->_remoteEntries[index].name));self->_BrowseRemote();}}});
        view.upload.Click([weak=weak_from_this()](auto&&,auto&&){if(const auto self=weak.lock())self->_SubmitSftp(true);});view.download.Click([weak=weak_from_this()](auto&&,auto&&){if(const auto self=weak.lock())self->_SubmitSftp(false);});view.cancelBrowse.Click([weak=weak_from_this()](auto&&,auto&&){if(const auto self=weak.lock();self&&!self->_closed)self->_service.CancelBrowse();});
        _root.FindName(L"BackendNotice").as<TextBlock>().Text(L"SFTP: single regular files; existing key/agent authentication. Remote conflicts require a new name. Other backends remain unavailable.");
    }
    inline void WorkspaceController::_BrowseRemote()
    {
        if(_closed)return;
        try{if(!_sftpConnection)throw std::runtime_error("Choose an enabled saved SSH profile in Transfers settings");_service.Browse(*_sftpConnection,winrt::to_string(_remotePath.Text()));_SftpError(L"Connecting...");}
        catch(const std::exception& error){_SftpError(winrt::to_hstring(error.what()));}
    }
    inline winrt::fire_and_forget WorkspaceController::_BrowseLocal()
    {
        if(_closed)co_return;
        const auto lifetime=shared_from_this();const auto generation=++_localGeneration;const auto ui=winrt::apartment_context();
        const std::filesystem::path directory{std::wstring(_localPath.Text())};std::vector<std::filesystem::path> entries;std::vector<bool> directories;std::wstring error;
        _localBusy=true;co_await winrt::resume_background();
        try{
            DirectoryLocks locked(directory);
            for(const auto& entry:std::filesystem::directory_iterator(directory)){if(entries.size()>=10000)throw std::runtime_error("Local directory exceeds the preview listing limit");const auto attrs=GetFileAttributesW(entry.path().c_str());if(attrs==INVALID_FILE_ATTRIBUTES||(attrs&FILE_ATTRIBUTE_REPARSE_POINT))continue;entries.push_back(entry.path());directories.push_back((attrs&FILE_ATTRIBUTE_DIRECTORY)!=0);}
        }catch(const std::exception& failure){error=winrt::to_hstring(failure.what());}
        co_await ui;if(_closed||generation!=_localGeneration)co_return;_localBusy=false;
        if(!error.empty()){_SftpError(winrt::hstring(error));co_return;}_view.localEmpty.Text(L"No files in this directory.");_localDirectory=directory.wstring();_localEntries=std::move(entries);_localDirectories=std::move(directories);_localFiles.Items().Clear();
        for(size_t i=0;i<_localEntries.size();++i){TextBlock row;row.Text((_localDirectories[i]?L"[Folder] ":L"")+_localEntries[i].filename().wstring());_localFiles.Items().Append(row);}
    }
    inline void WorkspaceController::_SubmitSftp(bool upload)
    {
        if(_closed)return;
        try{
            if(!_sftpConnection||_localBusy)throw std::runtime_error("Choose an enabled SSH profile and finish directory browsing first");
            if(winrt::to_string(_remotePath.Text())!=_remoteDirectory||std::wstring(_localPath.Text())!=_localDirectory)throw std::runtime_error("Browse both displayed directories before transferring");
            Request request;request.connection=*_sftpConnection;request.direction=upload?Direction::Upload:Direction::Download;
            auto name=winrt::to_string(_transferName.Text());
            if(upload){const auto index=_localFiles.SelectedIndex();if(index<0||static_cast<size_t>(index)>=_localEntries.size()||_localDirectories[index])throw std::runtime_error("Select a regular local file");request.local=_localEntries[index].wstring();if(name.empty())name=winrt::to_string(winrt::hstring(_localEntries[index].filename().wstring()));if(_remoteDirectory.empty())throw std::runtime_error("Browse the remote destination first");request.remote=_remoteDirectory+"/"+name;}
            else{const auto index=_remoteFiles.SelectedIndex();if(index<0||static_cast<size_t>(index)>=_remoteEntries.size()||!_remoteEntries[index].attributes.Regular())throw std::runtime_error("Select a regular remote file");if(name.empty())name=_remoteEntries[index].name;if(_localDirectory.empty())throw std::runtime_error("Browse the local destination first");request.local=(std::filesystem::path(_localDirectory)/std::wstring(winrt::to_hstring(name))).wstring();request.remote=_remoteDirectory+"/"+_remoteEntries[index].name;}
            if(!SafeLocalName(name))throw std::runtime_error("Choose a filename without path separators or Windows reserved characters");
            _service.Submit(std::move(request));_SftpError(L"Transfer started. Open Transfers to follow progress.");
        }catch(const std::exception& error){_SftpError(winrt::to_hstring(error.what()));}
    }
    inline winrt::fire_and_forget WorkspaceController::_ReplaceSftp(uint64_t job,uint64_t decision)
    {
        if(_closed)co_return;
        const auto lifetime=shared_from_this();
        try{
            std::wstring destination;for(const auto& item:_service.Jobs())if(item.id==job&&item.decision==decision&&item.phase==JobPhase::AwaitingDecision)destination=item.request.local;
            if(destination.empty()){Error(L"That replacement decision expired");co_return;}
            ContentDialog dialog;dialog.XamlRoot(_root.XamlRoot());dialog.Title(box_value(L"Replace this destination file?"));dialog.Content(box_value(L"Destination: "+destination+L"\n\nReplace the exact local file held by this transfer. Its existing contents are preserved until publication. This approval applies once."));dialog.PrimaryButtonText(L"Replace");dialog.CloseButtonText(L"Cancel");dialog.DefaultButton(ContentDialogButton::Close);
            if(co_await dialog.ShowAsync()==ContentDialogResult::Primary){if(_closed)co_return;if(!_service.Replace(job,decision))Error(L"That replacement decision expired; check the job again");}
        }catch(const std::exception& error){Error(winrt::to_hstring(error.what()));}catch(...){Error(L"Could not show the replacement confirmation");}
    }
    inline void WorkspaceController::_RenderSftp()
    {
        if(_closed)return;
        const auto browser=_service.Browser();
        if(!_sftpConnection||browser.connection!=*_sftpConnection){_Refresh();return;}
        if(browser.busy)_sftpStatus.Text(L"Connecting / browsing...");
        else if(!browser.error.empty())_SftpError(winrt::to_hstring(browser.error));
        else if(browser.generation&&browser.generation!=_renderedRemoteGeneration){_view.remoteEmpty.Text(L"No files in this directory.");_renderedRemoteGeneration=browser.generation;_remoteDirectory=browser.path;_remotePath.Text(winrt::to_hstring(browser.path));_remoteEntries=browser.entries;_remoteFiles.Items().Clear();for(const auto& item:_remoteEntries){TextBlock row;row.Text((item.attributes.Directory()?L"[Folder] ":L"")+winrt::to_hstring(item.name));_remoteFiles.Items().Append(row);}_sftpStatus.Text(L"Connected");}
        _Refresh();
    }
}
