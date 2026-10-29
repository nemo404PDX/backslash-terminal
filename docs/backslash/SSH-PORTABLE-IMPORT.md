# SSH selection and persistence

Settings > Add a new profile now offers **Scan this computer's SSH connections**, a selectable candidate list and **Import selected**. Startup/reload never scans host SSH configuration. Scanning does not add launch profiles. Import adds only selected connections to the Settings draft; ordinary Save commits through the existing atomic/stale-draft writer. Cancel discards the draft, and failed Save leaves disk unchanged and the draft available.

Selected imports are `backslash.sshConnections` records with `{ "mode": "hostAlias", "alias": "box" }`. They persist after restart without discovery. They use this computer's OpenSSH configuration, host trust and authentication when launched. They do not contain portable authentication or copy private keys, credentials, host configuration or trust files. On another computer, the same SSH alias and its dependencies must be configured there.

The existing resolved endpoint format (alias/host/user/port) and strict review adapter remain supported. Those launches retain `-F none`, explicit endpoint options and workspace trust behavior. Alias references and resolved endpoints share case-insensitive alias deduplication and the existing workspace profile identities. Hidden saved entries remain editable in Settings and are excluded from the launch dropdown; scans suppress their duplicates.

Compatibility: saving an edited transient discovery, including Hide, adopts it into the same SSH collection under its existing GUID and source. The saved profile retains its effective launch command and all explicit edits, including environment and advanced command overrides. Only generated launch fields are materialized; profile defaults remain inherited. Untouched discoveries and old preference maps are never automatically adopted. Old preference maps can prefill scanned candidates. Existing legacy saved profiles and resolved workspace imports are retained.

Failed scans clear stale rows, disable Import and show a message with the failing operation and available error details. Candidates remain outside the Settings draft until imported.

An edited legacy transient whose alias already has a saved resolved/reference owner now rejects Save/export with a clear conflict, retaining both the existing endpoint and all draft edits. Reviewed endpoint import explicitly transfers the legacy row's edits before marking them handled. DraftComparisonJson serializes raw draft state without ownership materialization, so adoption conflicts or the 512-record capacity limit cannot prevent dirty checks, Keep editing, Discard or Reset after a failed Save.

## Connection restrictions

An alias reference uses this computer's SSH configuration and authentication. A saved connection is not an authenticated or resumable session. Moving the portable copy does not move private keys, host configuration or trust files.

The separate reviewed-endpoint adapter accepts only simple explicit aliases and a restricted set of fields. Unsupported directives, advanced commands and names outside that adapter's allowed format require manual configuration; they are never silently shortened or ignored. Resolved-endpoint launches use explicit endpoint options with `-F none` rather than evaluating an alias through the host configuration.

Real SSH/SFTP authentication and broad interactive Settings coverage remain incomplete. [Contributor implementation and validation records](../dev/FEATURE-RECORDS.md#ssh-implementation-history-and-verification) describe the historical adapters and tests.
