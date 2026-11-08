# Transfers

Transfers provides a local/SFTP file browser and single-file uploads and downloads. It is experimental; verify important transfers and keep backups before replacing files.

Open Transfers from the main dropdown. Its default page shows shared activity and history, with search and status filters. Use the left menu to switch to the SFTP browser; backend fields are not part of the activity page. Both pages use a padded, centered content panel. The menu collapses in narrow windows.

Choose **Backend settings** in that menu to open [Settings > Transfers > Backends](TRANSFERS-BACKENDS.md), enable SFTP and select a saved SSH profile. Settings keeps its normal Save/Discard controls. Return to **SFTP**, browse both directories, select a regular file and choose Upload or Download. Open **Transfers** in the left menu to follow progress, completion and errors. Switching pages retains browser state and jobs. Stop requests cancellation; Retry creates a new attempt and may require a fresh overwrite decision.

Downloads refuse existing destinations unless you explicitly approve replacement of that exact local file. Uploads refuse an existing remote destination: choose a new filename. Recursive directory transfers, remote replacement, resume and persistent queues are not available. Cancellation or connection loss may leave a remote temporary file; the job reports its path when known. An uncertain publication is not reported as completed and cannot be blindly retried.

Settings > Interaction > Open Transfers in selects Tab, Side-by-side or Separate window. Reopening focuses the existing process-local view. Closing that view retains jobs; closing the application requires separate confirmation while transfers are active. Shutdown cancels work before publication and waits for publication already in progress. A timeout offers continued waiting or explicit unconfirmed termination. Crashes and forced OS termination cannot guarantee cleanup. History and jobs are not restored after process exit.

The view follows the application theme and can stack browser controls at narrow widths. Moving a Transfers pane to another window is currently restricted; close and reopen it with the desired placement. Ordinary downloads running inside terminal sessions remain separate child-process work.

See [backend and SSH requirements](TRANSFERS-BACKENDS.md) before connecting. Other backend toggles are configuration foundations only.
