# Repository layout and migration

The Git checkout root is distinct from the application/build root:

| Location | Contents |
| --- | --- |
| `README.md`, `LICENSE`, `NOTICE.md` | Project landing page/gallery and actual original legal/attribution files |
| `backslash/` | Application source, solutions, build imports/scripts, resources, samples, policies and dependency manifests |
| `docs/` | Backslash design/validation documentation, screenshots and user icon source exports |
| `.github/` | Workflows, issue/PR templates, and supported GitHub community guides |
| `.config/`, root Git/style files | Tool discovery/configuration that must remain discoverable from the checkout and nested source |
| `artifacts/` (ignored) | Existing and new build evidence/portable copies; no old artifact is relocated or overwritten |

The main solution is [backslash/OpenConsole.slnx](../../backslash/OpenConsole.slnx). The [build entry point](../../backslash/tools/backslash/README.md) accepts `-Rebuild`; run it by path from either the checkout root or another working directory. New compiler intermediates/outputs and NuGet packages are application-local (`backslash/obj`, `backslash/bin`, `backslash/packages`). This distinguishes a fresh/full build from old checkout-root caches. Old root generated/cache trees from the initial relocation are now preserved in a recoverable same-drive backup outside the checkout; the machine-local `artifacts/physical-cleanup-20261025/restore-map.json` records the exact location and hashes. Only verified-empty old directory shells were removed. All fifteen obsolete root paths must remain absent after builds; the repository-layout check enforces the actual physical layout, not only tracked-file counts. Existing portable artifacts stay in place.

Tracked files are relocated without discarding upstream history or changing product features. Existing README/gallery content, supplied screenshots/icon exports, duplicate `docs/images/icons_v1.zip`, actual root license and component notices are preserved. Community guides move to GitHub's supported `.github/` position. Provenance is [UPSTREAM-SOURCE.md](UPSTREAM-SOURCE.md). No final release version, installer/identity migration, tag, commit or push is selected by this restructuring.

MSBuild relative project imports stay within the application tree; packaging crosses explicitly to checkout-root docs/branding and legal files. Git-root discovery helpers now distinguish the checkout from `backslash/`. CI source/output paths and path filters use `backslash/`; root GitHub workflows remain under `.github/workflows`. Existing upstream Azure definitions still require their external service configuration and are not validated merely by moving files.

GUI validation is held by request while background work continues. Headless shader/renderer/model tests and portable resource/isolation checks do not alter live settings or user sessions.
