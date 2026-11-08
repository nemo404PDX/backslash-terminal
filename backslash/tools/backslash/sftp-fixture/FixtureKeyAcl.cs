using System;
using System.IO;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Security.AccessControl;
using System.Security.Principal;
using System.Security.Cryptography;
using Microsoft.Win32.SafeHandles;
public static class FixtureKeyAcl {
 [StructLayout(LayoutKind.Sequential)] struct Info { public uint Attributes; public System.Runtime.InteropServices.ComTypes.FILETIME Creation,Access,Write; public uint Volume,High,Low,Links,IndexHigh,IndexLow; }
 [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)] static extern SafeFileHandle CreateFileW(string path,uint access,uint share,IntPtr security,uint disposition,uint flags,IntPtr template);
 [DllImport("kernel32.dll",SetLastError=true)] static extern bool GetFileInformationByHandle(SafeFileHandle h,out Info info);
 static Info Identity(SafeFileHandle h) { Info i; if(!GetFileInformationByHandle(h,out i))throw new Win32Exception(); if((i.Attributes&0x410)!=0 || i.Links!=1)throw new IOException("Directory/reparse/multiple-link file refused");return i; }
 public static FileSecurity Descriptor(SecurityIdentifier sid) { var d=new FileSecurity();d.SetAccessRuleProtection(true,false);d.AddAccessRule(new FileSystemAccessRule(sid,FileSystemRights.FullControl,AccessControlType.Allow));return d; }
 public static bool Exact(FileSecurity d,SecurityIdentifier sid) { if(!d.AreAccessRulesProtected)return false;var rules=d.GetAccessRules(true,true,typeof(SecurityIdentifier));if(rules.Count!=1)return false;var r=(FileSystemAccessRule)rules[0];return !r.IsInherited && r.AccessControlType==AccessControlType.Allow && r.IdentityReference.Equals(sid) && r.FileSystemRights==FileSystemRights.FullControl && r.InheritanceFlags==InheritanceFlags.None && r.PropagationFlags==PropagationFlags.None; }
 public static void Guard(string path,string root) { path=Path.GetFullPath(path);root=Path.GetFullPath(root);if(!path.StartsWith(root+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))throw new IOException("Outside fixture");for(var p=path;!String.IsNullOrEmpty(p);p=Path.GetDirectoryName(p)) { if((File.Exists(p)||Directory.Exists(p)) && (File.GetAttributes(p)&FileAttributes.ReparsePoint)!=0)throw new IOException("Reparse ancestor refused"); } }
 public static void Protect(string path,string root,string expectedHash) {
  Guard(path,root);var sid=WindowsIdentity.GetCurrent().User;
  // GENERIC_READ | READ_CONTROL | WRITE_DAC; no WRITE_OWNER, ACCESS_SYSTEM_SECURITY or privilege adjustment.
  using(var h=CreateFileW(path,0x80060000,0,IntPtr.Zero,3,0x00200000,IntPtr.Zero)) {
   if(h.IsInvalid)throw new Win32Exception(Marshal.GetLastWin32Error());var before=Identity(h);
   using(var stream=new FileStream(h,FileAccess.Read)) {
    var old=FileSystemAclExtensions.GetAccessControl(stream);if(!old.GetOwner(typeof(SecurityIdentifier)).Equals(sid))throw new IOException("Owner mismatch");
    Func<string> hash=()=>{stream.Position=0;return Convert.ToHexString(SHA256.HashData(stream));};if(!String.Equals(hash(),expectedHash,StringComparison.OrdinalIgnoreCase))throw new IOException("Content changed before ACL");
    FileSystemAclExtensions.SetAccessControl(stream,Descriptor(sid));
    var after=FileSystemAclExtensions.GetAccessControl(stream);var end=Identity(h);
    if(!Exact(after,sid)||!after.GetOwner(typeof(SecurityIdentifier)).Equals(sid)||before.Volume!=end.Volume||before.IndexHigh!=end.IndexHigh||before.IndexLow!=end.IndexLow||!String.Equals(hash(),expectedHash,StringComparison.OrdinalIgnoreCase))throw new IOException("ACL/identity/content postcondition failed");
    Guard(path,root);
   }
  }
 }
 public static void MemoryTests() {
  var sid=WindowsIdentity.GetCurrent().User;var d=Descriptor(sid);if(!Exact(d,sid))throw new Exception("positive");
  if(Exact(d,new SecurityIdentifier(WellKnownSidType.WorldSid,null)))throw new Exception("wrong SID");
  var inherited=new FileSecurity();inherited.SetSecurityDescriptorSddlForm("D:P(A;ID;FA;;;"+sid.Value+")",AccessControlSections.Access);if(Exact(inherited,sid))throw new Exception("inherited ACE");
  d.SetAccessRuleProtection(false,false);if(Exact(d,sid))throw new Exception("inheritance");
  d=Descriptor(sid);d.AddAccessRule(new FileSystemAccessRule(new SecurityIdentifier(WellKnownSidType.WorldSid,null),FileSystemRights.Read,AccessControlType.Allow));if(Exact(d,sid))throw new Exception("extra ACE");
  d=new FileSecurity();d.SetAccessRuleProtection(true,false);d.AddAccessRule(new FileSystemAccessRule(sid,FileSystemRights.FullControl,AccessControlType.Deny));if(Exact(d,sid))throw new Exception("deny");
 }
}

