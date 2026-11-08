#Requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$PortableDirectory,
      [switch]$Observe,
      [switch]$TabStripOnly,
      [switch]$RequireUnclippedTabStrip)
$ErrorActionPreference='Stop'
$PSNativeCommandUseErrorActionPreference=$false
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$portable=(Resolve-Path -LiteralPath $PortableDirectory).Path
# Use actual current production components with matching generated projections.
if($RequireUnclippedTabStrip -and !$TabStripOnly){throw 'Strict tab-strip mode requires TabStripOnly'}
# Historical runtime comparison is confined to the tab-strip-only branch; full Settings gates still require current DLL hashes.
foreach($component in $(if($TabStripOnly){@()}else{@('Settings.Editor','Settings.Model')})){
    $name="Microsoft.Terminal.$component.dll"
    $built=Join-Path $repo "bin\x64\Release\Microsoft.Terminal.$component\$name"
    if((Get-FileHash -LiteralPath (Join-Path $portable $name)).Hash -ne (Get-FileHash -LiteralPath $built).Hash){throw "Use a runtime matching the current built $name"}
}
$work=Join-Path (Split-Path $repo -Parent) ('artifacts\ssh-scan-ui\'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'-'+[guid]::NewGuid().ToString('N').Substring(0,8))
$fixture=Join-Path $work 'portable'
New-Item -ItemType Directory -Path $fixture,(Join-Path $work 'synthetic-user\.ssh'),(Join-Path $work 'synthetic-system\ssh'),(Join-Path $work 'synthetic-programs\OpenSSH')|Out-Null
# No settings, state, Pins, keys or host files from the source runtime.
Get-ChildItem -LiteralPath $portable -Force|Where-Object Name -NE 'settings'|Copy-Item -Destination $fixture -Recurse
@('Host scan-fixture-one',' HostName never-connect.invalid',' IdentityFile never-read-private-key',' ProxyCommand never-executed')|Set-Content -LiteralPath (Join-Path $work 'synthetic-user\.ssh\config')
@('Host scan-fixture-two',' HostName never-connect.invalid')|Set-Content -LiteralPath (Join-Path $work 'synthetic-system\ssh\ssh_config')
# The scanner only tests file existence. This is deliberately not executable.
'NOT EXECUTABLE - isolated scan fixture only'|Set-Content -LiteralPath (Join-Path $work 'synthetic-programs\OpenSSH\ssh.exe')
Import-Module 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
Enter-VsDevShell -VsInstallPath 'C:\Program Files\Microsoft Visual Studio\18\Community' -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
$exe=Join-Path $fixture 'SshScanPageTests.exe'
& cl.exe /nologo /EHsc /std:c++20 /W4 /WX /MT "/I$repo\src\cascadia\TerminalApp\Generated Files" "/I$repo\src\cascadia\TerminalSettingsEditor" (Join-Path $PSScriptRoot 'SshScanPageTests.cpp') "/Fe:$exe" "/Fo:$work\SshScanPageTests.obj" /link windowsapp.lib user32.lib windowscodecs.lib /MANIFEST:EMBED "/MANIFESTINPUT:$repo\src\cascadia\WindowsTerminal\WindowsTerminal.manifest" *> (Join-Path $work 'build.log')
if($LASTEXITCODE){Get-Content (Join-Path $work 'build.log') -Tail 20;throw 'Native SSH Scan regression build failed'}
# Invokes only controls in its own offscreen XAML host; never sends input to
# existing windows, starts a terminal/SSH connection or restarts the user's app.
$arguments=@($work)
if($Observe){$arguments+='observe'}
if($TabStripOnly){if($Observe){throw 'Choose one probe mode'};$arguments+='tabstrip';if($RequireUnclippedTabStrip){$arguments+='strict'}}
& $exe @arguments *> (Join-Path $work 'scan-page.log')
$code=$LASTEXITCODE
Write-Output "SSH Scan UI exit: $code"
Get-Content (Join-Path $work 'scan-page.log')
Write-Output "SSH Scan UI evidence: $work"
exit $code
