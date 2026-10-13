#Requires -Version 7.0
[CmdletBinding()]
param([Parameter(Mandatory)][string]$EvidenceDirectory, [switch]$Runtime, [string]$FxcPath = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\fxc.exe')
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$evidence = (Resolve-Path -LiteralPath $EvidenceDirectory).Path
$asm = Join-Path $evidence 'retro-shader.asm'
$cso = Join-Path $evidence 'retro-shader.cso'
if ((Test-Path -LiteralPath $asm) -or (Test-Path -LiteralPath $cso)) { throw 'Fresh shader evidence paths required.' }
& $FxcPath /nologo /T ps_4_0 /E main /Fo $cso /Fc $asm (Join-Path $repo 'src\renderer\atlas\custom_shader_ps.hlsl')
if ($LASTEXITCODE -ne 0) { throw "Shader compiler failed: $LASTEXITCODE" }
$text = Get-Content -LiteralPath $asm -Raw
function Require([string]$Pattern,[string]$Message) {
    if ($text -notmatch $Pattern) { throw $Message }
    Write-Output "PASS: $Message"
}
Require 'PixelShaderSettings\s+cbuffer[^\r\n]+cb0\s+1' 'External ABI remains at b0'
Require 'RetroShaderSettings\s+cbuffer[^\r\n]+cb1\s+1' 'Built-in intensities bind at b1'
Require 'float time;\s+// Offset:\s+0 Size:\s+4' 'b0 time offset 0'
Require 'float scale;\s+// Offset:\s+4 Size:\s+4' 'b0 scale offset 4'
Require 'float2 resolution;\s+// Offset:\s+8 Size:\s+8' 'b0 resolution offset 8'
Require 'float4 background;\s+// Offset:\s+16 Size:\s+16' 'b0 background ends at byte 32'
Require 'float glowIntensity;\s+// Offset:\s+0 Size:\s+4' 'b1 glow offset 0'
Require 'float scanlineIntensity;\s+// Offset:\s+4 Size:\s+4' 'b1 scanline offset 4'
Require 'float2 retroPadding;\s+// Offset:\s+8 Size:\s+8' 'b1 padding ends at byte 16'
Require 'if_nz' 'Compiled shader retains conditional effect branches'

if ($Runtime) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vs = (& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath).Trim()
    if (-not $vs) { throw 'Existing C++ toolchain required for WARP runtime checks.' }
    Import-Module (Join-Path $vs 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
    Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
    $exe = Join-Path $evidence 'RetroShaderTests.exe'
    $obj = Join-Path $evidence 'RetroShaderTests.obj'
    if ((Test-Path -LiteralPath $exe) -or (Test-Path -LiteralPath $obj)) { throw 'Fresh runtime evidence paths required.' }
    & cl.exe /nologo /EHsc /std:c++20 /W4 /WX /MT (Join-Path $PSScriptRoot 'RetroShaderTests.cpp') "/Fe:$exe" "/Fo:$obj" /link d3d11.lib d3dcompiler.lib
    if ($LASTEXITCODE -ne 0) { throw "Runtime test compile failed: $LASTEXITCODE" }
    & $exe (Join-Path $repo 'src\renderer\atlas\custom_shader_vs.hlsl') (Join-Path $repo 'src\renderer\atlas\custom_shader_ps.hlsl')
    if ($LASTEXITCODE -ne 0) { throw "Runtime shader tests failed: $LASTEXITCODE" }
}
