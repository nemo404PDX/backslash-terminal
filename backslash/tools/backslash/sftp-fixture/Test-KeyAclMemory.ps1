# Compilation and memory-only predicates. Does not call Protect or touch any file ACL.
$ErrorActionPreference='Stop'
Add-Type -Path (Join-Path $PSScriptRoot 'FixtureKeyAcl.cs')
[FixtureKeyAcl]::MemoryTests()
'ACL descriptor memory predicates passed'
