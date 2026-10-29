# Shared pure validation and raw registry reading for host deployment/isolated tests.
function Initialize-HostRawRegistryReader {
    if('BackslashHostRawRegistry' -as [type]){return}
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.Win32;
public sealed class BackslashHostRawValue {
    public uint Kind;
    public byte[] Bytes;
}
public static class BackslashHostRawRegistry {
    [DllImport("advapi32.dll", EntryPoint="RegQueryValueExW", CharSet=CharSet.Unicode, ExactSpelling=true)]
    static extern int Query(IntPtr key, string name, IntPtr reserved, out uint kind, [Out] byte[] data, ref uint size);
    public static BackslashHostRawValue Read(RegistryKey key, string name) {
        uint kind, size=0;
        int result=Query(key.Handle.DangerousGetHandle(),name,IntPtr.Zero,out kind,null,ref size);
        if(result!=0 || size>131072) throw new InvalidOperationException("Raw registry read failed or exceeded bound; preserved.");
        uint expected=size;
        byte[] data=new byte[size];
        result=Query(key.Handle.DangerousGetHandle(),name,IntPtr.Zero,out kind,data,ref size);
        if(result!=0 || size!=expected) throw new InvalidOperationException("Registry changed during raw read; preserved.");
        return new BackslashHostRawValue { Kind=kind, Bytes=data };
    }
    public static string Decode(uint kind, byte[] data) {
        if(kind!=1 || data==null || data.Length<2 || data.Length>131072 || data.Length%2!=0)
            throw new InvalidOperationException("Unexpected REG_SZ kind or byte length; preserved.");
        if(data[data.Length-1]!=0 || data[data.Length-2]!=0)
            throw new InvalidOperationException("REG_SZ missing terminator; preserved.");
        string text=new UnicodeEncoding(false,false,true).GetString(data,0,data.Length-2);
        if(text.IndexOf('\0')>=0) throw new InvalidOperationException("REG_SZ contains embedded/repeated terminator; preserved.");
        return text;
    }
}
'@
}
function Assert-ExpectedRawConfig($Values,[int]$ChildCount,[hashtable]$Expected) {
    if($ChildCount){throw 'Target has unexpected children; preserved.'}
    $decoded=[Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
    foreach($entry in $Values){
        if($entry.Name -cnotin @('Owner','SchemaVersion','TargetExecutable','SupportRoot') -or $decoded.ContainsKey($entry.Name)){throw 'Unknown/duplicate target value; preserved.'}
        $text=[BackslashHostRawRegistry]::Decode($entry.Kind,$entry.Bytes)
        if($text -cne $Expected[$entry.Name]){throw 'Changed target value; preserved.'}
        $decoded.Add($entry.Name,$text)
    }
    if(!$decoded.ContainsKey('Owner') -or $decoded['Owner'] -cne $Expected.Owner){throw 'Unmarked target; preserved.'}
}
function Assert-HostPackageLocation($Package,[string]$Support) {
    # Loose -Register must be a development registration rooted at our exact manifest.
    # Identity alone does not prove ownership. Missing/other deployment forms fail closed.
    if($Package.IsDevelopmentMode -isnot [bool] -or !$Package.IsDevelopmentMode){throw 'Package is not our loose development registration; preserved.'}
    if($Package.InstallLocation -isnot [string] -or $Package.InstallLocation -notmatch '^[A-Za-z]:\\'){throw 'Package install location unavailable/unsupported; preserved.'}
    $actual=[IO.Path]::GetFullPath($Package.InstallLocation).TrimEnd([char]'\')
    $expected=[IO.Path]::GetFullPath($Support).TrimEnd([char]'\')
    if(![StringComparer]::OrdinalIgnoreCase.Equals($actual,$expected)){throw 'Package identity exists at another install location; preserved.'}
}
