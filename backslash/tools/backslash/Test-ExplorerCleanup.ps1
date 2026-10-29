#Requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$EvidenceDirectory)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$evidence = (Resolve-Path -LiteralPath $EvidenceDirectory).Path
$cleanup = Join-Path $PSScriptRoot 'Remove-ExplorerIntegration.ps1'
$id = [guid]::NewGuid()
$rootPath = 'Software\Backslash.ExplorerIntegration.Tests\Cleanup-' + $id.ToString('D')
$hive = [Microsoft.Win32.Registry]::CurrentUser
if ($null -ne $hive.OpenSubKey($rootPath)) { throw 'Fresh isolated root required.' }
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.Win32;
public static class BackslashCleanupTestRaw {
    [DllImport("advapi32.dll", EntryPoint="RegSetValueExW", CharSet=CharSet.Unicode, ExactSpelling=true)]
    static extern int RegSetValueEx(IntPtr key,string name,uint reserved,uint type,IntPtr data,uint length);
    [DllImport("advapi32.dll", EntryPoint="RegQueryValueExW", CharSet=CharSet.Unicode, ExactSpelling=true)]
    static extern int RegQueryValueEx(IntPtr key,string name,IntPtr reserved,out uint type,[Out] byte[] data,ref uint length);
    public static void Write(RegistryKey key,string name,uint type,byte[] data) {
        IntPtr buffer=Marshal.AllocHGlobal(Math.Max(1,data.Length));
        try {
            Marshal.Copy(data,0,buffer,data.Length);
            int code=RegSetValueEx(key.Handle.DangerousGetHandle(),name,0,type,buffer,(uint)data.Length);
            if(code!=0) throw new InvalidOperationException("Test write failed: "+code);
        } finally { Marshal.FreeHGlobal(buffer); }
    }
    public static string Snapshot(RegistryKey key) {
        var output=new StringBuilder();
        string[] names=key.GetValueNames(); Array.Sort(names,StringComparer.Ordinal);
        foreach(string name in names) {
            uint type,size=0;
            int code=RegQueryValueEx(key.Handle.DangerousGetHandle(),name,IntPtr.Zero,out type,null,ref size);
            if(code!=0) throw new InvalidOperationException("Test query failed: "+code);
            byte[] data=new byte[size];
            code=RegQueryValueEx(key.Handle.DangerousGetHandle(),name,IntPtr.Zero,out type,data,ref size);
            if(code!=0) throw new InvalidOperationException("Test query failed: "+code);
            output.Append(name.Length).Append(':').Append(name).Append(':').Append(type).Append(':').Append(Convert.ToBase64String(data)).Append(';');
        }
        string[] children=key.GetSubKeyNames(); Array.Sort(children,StringComparer.Ordinal);
        foreach(string name in children) using(var child=key.OpenSubKey(name)) output.Append(name).Append('{').Append(Snapshot(child)).Append('}');
        return output.ToString();
    }
}
'@
$owner = 'Backslash.Explorer.v1.{BF687475-455F-4EA3-AC14-F3FDDF06B825}'
$surfaces = @('Directory\shell\Backslash.Open', 'Directory\Background\shell\Backslash.Open')
function Require([bool]$condition, [string]$message) { if (-not $condition) { throw $message } }
$context = 'sentinel safeguards'
function Reject([scriptblock]$action) { $failed=$false; try { & $action | Out-Null } catch { $failed=$true }; Require $failed "Expected cleanup rejection: $context" }
function New-OwnedEntries {
    foreach ($surface in $surfaces) {
        $key = $hive.CreateSubKey("$rootPath\$surface")
        try {
            $key.SetValue('BackslashOwner',$owner,[Microsoft.Win32.RegistryValueKind]::String)
            $key.SetValue('BackslashExecutable','D:\Deleted Different Copy\Backslash.exe',[Microsoft.Win32.RegistryValueKind]::String)
            $key.SetValue('','Open in Backslash',[Microsoft.Win32.RegistryValueKind]::String)
            $key.SetValue('Icon','"D:\Deleted Different Copy\Backslash.exe",0',[Microsoft.Win32.RegistryValueKind]::String)
            $key.SetValue('MultiSelectModel','Single',[Microsoft.Win32.RegistryValueKind]::String)
            $command=$key.CreateSubKey('command')
            try { $command.SetValue('','"D:\Deleted Different Copy\Backslash.exe" --backslash-explorer-folder "%1\."',[Microsoft.Win32.RegistryValueKind]::String) } finally { $command.Dispose() }
        } finally { $key.Dispose() }
    }
}
$root = $hive.CreateSubKey($rootPath)
try {
    # The real standalone script refuses an absent or mismatched sentinel before
    # accessing either verb. Its production root is never selected by this test.
    Reject { & $cleanup -IsolatedTestId $id -Confirm:$false }
    $root.SetValue('BackslashCleanupTestOwner','wrong',[Microsoft.Win32.RegistryValueKind]::String)
    Reject { & $cleanup -IsolatedTestId $id -Confirm:$false }
    $root.SetValue('BackslashCleanupTestOwner',$id.ToString('D'),[Microsoft.Win32.RegistryValueKind]::String)
    & $cleanup -IsolatedTestId $id -Confirm:$false | Out-Null # missing entries/idempotency
    New-OwnedEntries
    $before = [BackslashCleanupTestRaw]::Snapshot($root)
    & $cleanup -IsolatedTestId $id -WhatIf | Out-Null
    Require ([BackslashCleanupTestRaw]::Snapshot($root) -ceq $before) 'WhatIf mutated registration.'
    & $cleanup -IsolatedTestId $id -Confirm:$false | Out-Null
    foreach ($surface in $surfaces) { Require ($null -eq $root.OpenSubKey($surface)) 'Owned entry was not removed.' }
    & $cleanup -IsolatedTestId $id -Confirm:$false | Out-Null
    # Partially written owned entry can still be removed without an executable.
    $partial=$root.CreateSubKey($surfaces[0]); $partial.SetValue('BackslashOwner',$owner); $partial.Dispose()
    & $cleanup -IsolatedTestId $id -Confirm:$false | Out-Null
    $malformed = @(
        @{kind=2;data=[Text.Encoding]::Unicode.GetBytes($owner+[char]0)},
        @{kind=4;data=[byte[]](1,0,0,0)}, @{kind=3;data=[byte[]](1,2)}, @{kind=7;data=[byte[]](0,0,0,0)},
        @{kind=1;data=[byte[]]@()}, @{kind=1;data=[byte[]](1)},
        @{kind=1;data=[Text.Encoding]::Unicode.GetBytes('unterminated')},
        @{kind=1;data=[Text.Encoding]::Unicode.GetBytes('a'+[char]0+'b'+[char]0)}
    )
    $cases=0
    foreach ($surface in $surfaces) {
        foreach ($name in @('BackslashOwner','BackslashExecutable','','Icon','MultiSelectModel','command')) {
            foreach ($bad in $malformed) {
                $context = "$surface value [$name], kind $($bad.kind), bytes $($bad.data.Length)"
                New-OwnedEntries
                $target=if ($name -ceq 'command') { $root.OpenSubKey("$surface\command",$true) } else { $root.OpenSubKey($surface,$true) }
                try {
                    $valueName = if ($name -ceq 'command') { '' } else { $name }
                    [BackslashCleanupTestRaw]::Write($target, $valueName, $bad.kind, $bad.data)
                    Reject { [BackslashCleanupStrings]::Read($target, $valueName) }
                } finally { $target.Dispose() }
                $before=[BackslashCleanupTestRaw]::Snapshot($root)
                Reject { & $cleanup -IsolatedTestId $id -Confirm:$false }
                Require ([BackslashCleanupTestRaw]::Snapshot($root) -ceq $before) 'Malformed preflight changed either surface.'
                $cases++
            }
        }
    }
    # Foreign/unmarked entries and unknown values/children also preserve BOTH surfaces.
    foreach ($collision in @('foreign','unmarked','unknown-value','unknown-child','unknown-command-value','unknown-command-child')) {
        $context = $collision
        $hive.DeleteSubKeyTree("$rootPath\$($surfaces[1])",$false)
        New-OwnedEntries
        $target=$root.OpenSubKey($surfaces[1],$true)
        try {
            switch ($collision) {
                foreign { $target.SetValue('BackslashOwner','foreign') }
                unmarked { $target.DeleteValue('BackslashOwner') }
                unknown-value { $target.SetValue('ForeignData','preserve') }
                unknown-child { $child=$target.CreateSubKey('foreign'); $child.Dispose() }
                unknown-command-value { $child=$target.OpenSubKey('command',$true); $child.SetValue('foreign','preserve'); $child.Dispose() }
                unknown-command-child { $child=$target.CreateSubKey('command\foreign'); $child.Dispose() }
            }
        } finally { $target.Dispose() }
        $before=[BackslashCleanupTestRaw]::Snapshot($root)
        Reject { & $cleanup -IsolatedTestId $id -Confirm:$false }
        Require ([BackslashCleanupTestRaw]::Snapshot($root) -ceq $before) 'Collision preflight changed either surface.'
    }
    "PASS: actual standalone cleanup missing/partial/deleted-copy lifecycle, WhatIf, idempotency, sentinel safeguards, $cases malformed cases, six ownership/schema collisions; both surfaces byte-preserved" | Tee-Object -FilePath (Join-Path $evidence 'cleanup-tests.log')
} finally {
    $root.Dispose()
    # The path was generated above, checked absent, and is confined to this exact test namespace.
    $hive.DeleteSubKeyTree($rootPath,$false)
}
