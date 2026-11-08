# Transfers backends

Settings > Transfers > Backends enables SFTP and selects an existing saved SSH profile. **Backend settings** in the standalone Transfers menu opens this same Settings page. Save or discard edits through the normal Settings controls. Disabling a backend prevents new submissions without canceling active jobs.

SFTP accepts saved Backslash SSH profiles and saved legacy Windows Terminal SSH profiles, including hidden profiles. Deleted, transient or missing profiles cannot be used. Profiles are resolved again before a connection; changing or removing a profile can require selecting it again. Custom terminal command lines and unsupported profile shapes are refused rather than executed as arbitrary commands.

The adapter uses the selected profile's supported OpenSSH alias or structured endpoint with an absolute SSH executable. Existing SSH configuration supplies connection and authentication details. It requires noninteractive authentication and an already trusted host key. It does not prompt for passwords, accept unknown or changed host keys, update known hosts, forward an agent, or reuse SSH multiplexing masters. Configure and verify SSH access separately before using Transfers. Secrets are not copied into backend settings.

The current workflow browses directories and transfers individual regular files. Local destinations must be supported local drives; UNC, device and mapped/network destinations are refused. Remote names that cannot safely map to Windows filenames are refused. Remote replacement, recursive copies, password prompts, automatic reconnect/resume and durable history are unavailable. A selected profile does not prove server connectivity or compatibility; server and authentication support varies.

SABnzbd, aria2 and rclone settings retain enablement intent but provide no working transfer adapters or setup/install actions. No tool installation or remote credential configuration happens automatically.

Backend settings use `backslash.transferBackends`, version 1, with Boolean `enabled` entries and an SFTP saved-profile GUID. Malformed or unsupported configurations are preserved on unrelated saves and shown as errors instead of silently replaced.
