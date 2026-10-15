# Current pinned-tabs checkpoint - automatic live cwd

Fresh plain interactive local PowerShell 7 reports its actual filesystem-provider cwd automatically. The integration retains the original prompt and logical launcher, refreshes at input readiness, clears during execution/unsupported providers/disconnect and invalidates old connection generations. **Pin current folder** requires a usable report; **Add a folder...** is separate. See [repair, reusable metadata architecture, limitations and acceptance](LIVE-CWD-AND-SESSION-METADATA.md).

* App: **0 errors / 134 warnings**. **230/230 Settings**, **34/34 Control**, **759 parser passed / 1 preexisting skip**, **9/9 portable runtime**, isolation passed.
* Real owned PowerShell: home -> D: -> C:, three fixed pins, spaces/semicolons/Unicode/apostrophes, custom PSDrive, nested execution, unsupported provider, custom prompt observing `$?`/`$LASTEXITCODE`, prior handler and idempotence.
* Native menu/dialog/move/unpin/close/restoration/accessibility/DPI checks remain unrun; supported native CUA is unavailable.
* Executable: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-20261015-101041-982-Release-x64-35414271/WindowsTerminal.exe`.
* Complete clean ZIP: `D:/codex-workspace/backslash-terminal/artifacts/portable/Backslash-Dev-0.0.1.0-PinsLiveCwd-x64-35414271.zip`; SHA256 `18CA17828C33E23E0A0CC5AF2E584061E01ADF98442C53C81B6BE129DD2C4168`.
* Evidence/source checkpoint: `artifacts/build/20261015-101041-982-Release-x64-35414271`. Previous identity and intermediate builds are retained.

This repository checkpoint is based on dc027cae; the validated portable was built before the checkpoint commit. Latest native live-cwd menu/dialog acceptance is still pending. No persistent shell-profile/user-working-settings/SSH-config changes, remote execution, registration or install occurred. Future status bars, transfer consumers, success-filtered history, hidden-file defaults and Unix-style functions are design only in the linked document. Legacy OSC output is unverified, not authorization.
