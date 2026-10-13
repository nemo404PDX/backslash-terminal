# Curated terminal font catalog

[Project entry point](README.md) · [Roadmap](ROADMAP.md)

## Requested behavior

Add a **Get more fonts** button in profile font/Appearance settings that opens a curated modal/catalog of good terminal fonts. Prioritize and label ligature support. Store upstream links and metadata in maintainable catalog data; do not bundle font files or scatter hardcoded links across UI components.

A user explicitly clicks **Install** to fetch a selected font from its official repository/source. No silent downloads. No fonts have been selected, downloaded or installed in this documentation task.

## Proposed UX

Show a code preview with `!=`, `=>` and `===`, plus readable ordinary text. Include license, official source, pinned version, installed/download status and an apply-to-profile action. Preview layout should remain monospaced and communicate whether sample ligatures are enabled.

Offer an understandable per-profile ligatures toggle using existing font feature support where suitable. The baseline [FontConfig.cpp](../../src/cascadia/TerminalSettingsModel/FontConfig.cpp), [FontConfig.idl](../../src/cascadia/TerminalSettingsModel/FontConfig.idl) and [MTSMSettings.h](../../src/cascadia/TerminalSettingsModel/MTSMSettings.h) expose a font feature map. A simple toggle must map deliberately to actual font OpenType features rather than assuming every font uses identical tags; exact behavior needs validation.

## Download and license requirements

Use an explicit catalog entry with official source URL, version, downloadable font assets, license/notice references and verified hashes. Validate source/version/hash before accepting a download. Reject arbitrary executable installers; do not execute downloaded code. Keep applicable font license notices with fetched assets: external fetching does not remove license obligations.

Handle offline catalog browsing, unavailable sources, failed/hash-mismatched downloads, unsupported font formats, permissions and already installed versions clearly. Do not apply a font until its installation/availability is confirmed; preserve the previous usable profile font on errors.

## Open decisions and next work

Per-user Windows installation versus app-private font registration is an open implementation decision, including permissions, uninstall/updates, cache invalidation and discovery by the renderer. Decide catalog schema/update trust, hash maintenance, preview before download, licensing attribution placement and version conflict behavior.

Research the specific curated fonts, ligature behavior, official release assets and individual licenses before implementation. Potential examples discussed later must not be presented as selected entries. No font catalog has been researched or populated in this task.

## Future acceptance checks

Explicit Install is the only download trigger; official pinned assets pass integrity checks; errors/offline state do not break profiles; installed status is accurate; apply targets only the selected profile; ligatures and text alignment behave correctly; license notices remain available. Test the selected installation scope and clean recovery without running executable installers.


## Representative preview groups

Do not rely on a generic alphabet-only sample. Provide programming operators/ligatures (including !=, => and ===), ambiguous glyphs (0/O and 1/l/I), box drawing, and prompt symbols. Label symbols that require patched glyph coverage and expose fallback/missing-glyph behavior rather than implying every font includes them. Include editable sample text for the user's own code/prompt and a ligatures enabled/disabled comparison. Exact samples and font coverage still need evaluation. The user did not identify which prior application's sample was inadequate; do not attribute this issue to a specific product.

## Requested: prompt theme glyph coverage

Support and preview extra glyphs used by Oh My Posh and similar prompt themes: Git branch/status icons, folder icons, language/environment symbols and Powerline segments. Ligature support and Nerd Font/Powerline/patched glyph coverage are separate capabilities; neither implies the other. The catalog should badge them separately, with exact font version and coverage verified before making claims.

Provide a realistic prompt preview containing a folder, Git branch, dirty state and language/environment segment. Include visible missing-glyph, fallback and cell-spacing indicators, especially at Powerline joins and icon/text boundaries. Preview both ordinary text fallback and the intended glyph rendering. Exact symbol codepoints must be verified against the chosen font/version rather than inferred from a font's name.

The shell/prompt engine controls environment-specific prompt content; the terminal font renders the characters it receives. Installing or applying a font does not automatically install, enable or configure Oh My Posh or another prompt engine. No prompt engine installation/configuration is authorized or performed in this task.

Future acceptance checks: independent capability badges are accurate; prompt symbols and Powerline joins align in terminal cells; missing glyphs and fallback are understandable; ligature toggling does not imply patched coverage; applying a font leaves shell/prompt configuration unchanged.
