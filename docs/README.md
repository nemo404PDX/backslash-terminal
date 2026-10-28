# Backslash documentation

For the published preview versus current uncommitted work, start with [release and development status](backslash/DEVELOPMENT-STATUS.md). It links current Settings/SSH/rendering records and distinguishes automated verification from pending visual QA.

Start with the [Backslash guide](backslash/README.md) for features, implementation evidence and future designs. The [root README](../README.md) gives the product overview; [tools/backslash/README.md](../backslash/tools/backslash/README.md) covers Windows builds and validation.

| Location | Ownership and purpose |
| --- | --- |
| [backslash/](backslash/README.md) | Backslash behavior, implementation records, research and proposed features. |
| [../doc/](../backslash/doc/README.md) | Inherited Windows Terminal technical references, original specifications and Microsoft process/history. Some files are maintained by Backslash. |
| [images/](images/README.md) | Screenshots and working badge artwork. The three SVG badges are build inputs; keep their paths stable. |
| [asset-backups/](asset-backups/README.md) | Original supplied artwork and hash manifest. |
| [../branding/concepts/](../backslash/branding/concepts/README.md) | Backslash visual direction and asset preparation history. |

A file named PLAN or DESIGN may include implemented follow-ups. Read its status and evidence before treating it as unfinished. Historical artifact paths and test counts describe recorded checkpoints, not fresh validation of this checkout.

## Upstream updates

The baseline is recorded in [UPSTREAM-SOURCE.md](backslash/UPSTREAM-SOURCE.md). Keep technical-reference paths stable when incorporating upstream changes. Six historical planning/process files now live in [doc/upstream/history/](../backslash/doc/upstream/history/README.md), which records the original-to-current path mapping. Review incoming documentation for applicability; keep fork guidance in docs/backslash/ and classify inherited references/history in [doc/README.md](../backslash/doc/README.md). The settings schema in doc/cascadia/ is maintained shared material, not an archive.

Keep [LICENSE](../LICENSE), [NOTICE.md](../NOTICE.md), component licenses/notices and attribution in place. Source-local, sample and tool READMEs stay beside their code. Root contributor, support, security and conduct documents retain upstream text with provenance labels; Microsoft contacts are not a promise of support for Backslash.