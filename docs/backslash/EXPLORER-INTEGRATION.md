# Explorer integration

Development builds can add **Open in Backslash** to the main Windows 11 folder and folder-background menu. Integration is optional and applies to your Windows user. Opening another Backslash copy never changes the selected target automatically.

## Enable or choose a copy

Open **Settings → Startup → Explorer integration**. The main card shows modern registration status and the relevant actions:

- **Enable / Use this copy** selects the running portable copy after confirmation. If another copy is selected, the confirmation shows both paths.
- **Repair** checks the owned integration, restores missing reviewed helper files and repairs missing registration or target data. It preserves the selected copy. Use **Use this copy** explicitly to replace an unavailable target.
- **Remove** removes the owned modern helper registration, target selection and verified support files. Other applications and classic menu entries are left unchanged.

These actions apply immediately, independently of Settings Save/Discard. The application does not alter profile starting directories or require profiles to use `.`. A folder-menu invocation overrides the starting directory only for that launch; shell startup scripts can still choose a different directory.

This development helper requires Windows 11, an unpackaged x64 portable build with the complete helper payload, and Developer Mode already enabled when registration is needed. Backslash does not enable it, import certificates or request elevation. A signed end-user distribution is not included. The terminal itself remains portable and unpackaged.

## Details and troubleshooting

**Details** is collapsed by default. It shows the selected executable, support path, exact development package and classic-menu status. Actionable errors remain visible in the main card.

- **Check status** rereads registration without changing it.
- **Refresh shell** sends a shell update request. It does not restart Explorer or guarantee an immediate taskbar-icon refresh.
- **Restart Explorer…** is a separate, optional troubleshooting action with confirmation and a disruption warning. It can interrupt the desktop, taskbar, Explorer windows and file operations. Windows can refuse a safe restart; Backslash never forces termination or elevates.

The stable helper lives under `%LOCALAPPDATA%\Backslash\ExplorerIntegration\Dev`. Target selection is stored under `HKCU\Software\Backslash\ExplorerIntegration`; it does not follow another portable copy unless you explicitly choose one. Moving or deleting the selected portable folder can require retargeting. Replacing a complete copy at the same path retains that selection.

Foreign/unmarked data, unexpected files, changed helper bytes, unsupported package identities and reparse paths are preserved with an error. Missing owned data can be repaired; changed or unknown data may need manual inspection. Windows-managed package metadata is never recursively deleted. A partial removal or rollback error reports the remaining state rather than claiming success.

Folder launch accepts accessible absolute filesystem directories, including spaces and Unicode. Files, nonexistent folders and unsupported shell/device locations produce errors. Network-directory behavior also depends on the chosen shell.

## Older classic entries

Older builds register **Open in Backslash** under **Show more options**. Modern status/actions do not modify those entries. Their status is shown separately in Details. The standalone `Remove-ExplorerIntegration.ps1` in the portable folder removes only the two owned classic entries; it does not uninstall the modern helper.

[Developer architecture and validation](../dev/EXPLORER-MODERN-INVESTIGATION.md)
