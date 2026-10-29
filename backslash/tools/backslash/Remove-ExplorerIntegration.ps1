#Requires -Version 7.0
[CmdletBinding(SupportsShouldProcess, ConfirmImpact='High')]
param([guid]$IsolatedTestId = [guid]::Empty)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# Standalone recovery: no executable or profile is needed. Never wildcard-delete Classes.
$owner = 'Backslash.Explorer.v1.{BF687475-455F-4EA3-AC14-F3FDDF06B825}'
$paths = @('Software\Classes\Directory\shell\Backslash.Open', 'Software\Classes\Directory\Background\shell\Backslash.Open')
# Raw Win32 reads deliberately avoid RegistryKey.GetValue's string normalization.
# This standalone script validates kinds, byte lengths, terminators and embedded NULs.
if (-not ('BackslashCleanupStrings' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using Microsoft.Win32;
public static class BackslashCleanupStrings {
    [DllImport("advapi32.dll", EntryPoint="RegQueryValueExW", CharSet=CharSet.Unicode, ExactSpelling=true)]
    static extern int RegQueryValueEx(IntPtr key, string name, IntPtr reserved, out uint type, [Out] byte[] data, ref uint size);
    public static string Read(RegistryKey key, string name) {
        uint type, size=0;
        int result=RegQueryValueEx(key.Handle.DangerousGetHandle(), name, IntPtr.Zero, out type, null, ref size);
        if(result==2) return null;
        if(result!=0) throw new InvalidOperationException("Registry read failed: " + result);
        if(type!=1 || size<2 || size>131072 || size%2!=0) throw new InvalidOperationException("Unexpected registry string kind or byte length; untouched.");
        uint expected=size;
        byte[] data=new byte[size];
        result=RegQueryValueEx(key.Handle.DangerousGetHandle(), name, IntPtr.Zero, out type, data, ref size);
        if(result!=0 || type!=1 || size!=expected) throw new InvalidOperationException("Registry string changed during read; untouched.");
        if(data[data.Length-1]!=0 || data[data.Length-2]!=0) throw new InvalidOperationException("Registry string missing terminator; untouched.");
        string value=System.Text.Encoding.Unicode.GetString(data,0,data.Length-2);
        if(value.IndexOf('\0')>=0) throw new InvalidOperationException("Embedded NUL in registry string; untouched.");
        return value;
    }
}
'@
}
# A test can select only a GUID-named child of this fixed disposable namespace,
# with an exact matching sentinel. There is no arbitrary registry-root parameter.
if ($IsolatedTestId -ne [guid]::Empty) {
    $root = 'Software\Backslash.ExplorerIntegration.Tests\Cleanup-' + $IsolatedTestId.ToString('D')
    $sentinel = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey($root)
    if ($null -eq $sentinel) { throw 'Isolated test sentinel missing; no changes.' }
    try { if ([BackslashCleanupStrings]::Read($sentinel, 'BackslashCleanupTestOwner') -cne $IsolatedTestId.ToString('D')) { throw 'Isolated test sentinel mismatch; no changes.' } } finally { $sentinel.Dispose() }
    $paths = @("$root\Directory\shell\Backslash.Open", "$root\Directory\Background\shell\Backslash.Open")
}
function Assert-OwnedShape($key) {
    if ([BackslashCleanupStrings]::Read($key, 'BackslashOwner') -cne $owner) { throw 'Foreign/unmarked Backslash.Open collision; nothing removed.' }
    foreach ($name in $key.GetValueNames()) {
        if ($name -cnotin @('', 'BackslashOwner', 'BackslashExecutable', 'Icon', 'MultiSelectModel')) { throw 'Unknown registry value; entry left untouched.' }
        $null = [BackslashCleanupStrings]::Read($key, $name)
    }
    foreach ($name in $key.GetSubKeyNames()) {
        if ($name -cne 'command') { throw 'Unknown registry subkey; entry left untouched.' }
    }
    $command = $key.OpenSubKey('command')
    if ($null -ne $command) {
        try {
            if ($command.GetSubKeyNames().Count -ne 0 -or @($command.GetValueNames() | Where-Object { $_ -cne '' }).Count -ne 0) { throw 'Unknown command data; entry left untouched.' }
            foreach ($name in $command.GetValueNames()) { $null = [BackslashCleanupStrings]::Read($command, $name) }
        } finally { $command.Dispose() }
    }
}
# Preflight both before asking or mutating. Each deletion also checks ownership again.
foreach ($path in $paths) {
    $key = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey($path)
    if ($null -eq $key) { Write-Output "Absent: HKCU\$path"; continue }
    try { Assert-OwnedShape $key; Write-Output "Owned: HKCU\$path -> $([BackslashCleanupStrings]::Read($key, 'BackslashExecutable'))" } finally { $key.Dispose() }
}
if ($PSCmdlet.ShouldProcess(($paths -join "`nHKCU\"), 'Remove only the owned Backslash Explorer entries under HKCU')) {
    foreach ($path in $paths) {
        $key = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey($path, $true)
        if ($null -eq $key) { continue }
        try { Assert-OwnedShape $key; $key.DeleteSubKey('command', $false) } finally { $key.Dispose() }
        [Microsoft.Win32.Registry]::CurrentUser.DeleteSubKey($path, $false)
    }
    Write-Output 'Owned Backslash entries removed. Reopen the Explorer context menu to refresh.'
}
