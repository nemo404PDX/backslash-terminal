# Phosphor regression audit: accepted 13ebff74 through pins 35414271

**Follow-up, 2026-10-16:** the user clarified the build pair and both running copies were inspected read-only. See [the resolved comparison](PHOSPHOR-COMPARISON-13ebff74-35414271.md) for the differing saved schemes, strengths, opacity and explicit tint values. The original audit below remains an earlier snapshot; its missing-identity conclusion is superseded.

Audit anchor: **960a9f976f08cd7d4c8dcff263c61b71afedb101**, 2026-10-15. This report records observations before the separately requested curated-defaults change. It does not identify the two unnamed builds in the user's complaint or claim a native screenshot reproduction.

## Findings

The recorded accepted baseline is build **13ebff74**, documented in CRT-RENDERING-PLAN.md's user acceptance checkpoint. Its phosphor shader source matches **dc027cae58b9414c4c0bdcb5790093fb7165ba8e**. Current HEAD at audit time is the later pins commit **960a9f9**.

* Accepted versus later pin source: `custom_shader_ps.hlsl`, Atlas backend/constants/API state, tint model defaults and serialization, color-picker implementation/XAML, editor tint bindings, inbox defaults and all three bezel helpers are unchanged. The adapter/ControlProperties/Core/TermControl files gained launch-identity and connection-cwd fields, not tint calculations or values.
* The accepted ZIP and all four preserved PinsPreview/CaptureFix/IdentityFix/LiveCwd ZIPs contain **byte-identical production phosphor shader data**, and in fact the same four complete DXBC containers. Exact phosphor DXBC SHA256: `EA9498337F61E5BE3B63C6F17F3FF5BC00554BF68AE8ABCA1B8816316595DE03`. This compares the stripped Release bytecode embedded in `Microsoft.Terminal.Control.dll`, not just source files or a test recompile. Whole DLL hashes differ due to other code; that is not a color-value change.
* Every examined delivery has the same `defaults.json` hash: `5ABE4FEFAF0D0D1CB160924BA55B26677E9C5C41EEC4B0AD3D2C57A0E3CCF504`. Examined ZIPs contain no saved `settings` directory. No evidence of a changed packaged phosphor RGB default, palette reset or new tint serialization was found after acceptance.
* The earlier **bfd6c42 -> dc027cae** shader diff is real and narrow: a 16-line diff replaces the black/phosphor/white luminance ramp with `Y * selectedLinearPhosphor`, and removes the near-zero alpha denominator clamp. It deliberately makes white glyphs retain the selected color at all opacity levels. That correction was already present in the accepted **13ebff74** build. Earlier PhosphorTint/Groups/Profiles ZIPs have the older bytecode, so comparing one of those to a corrected ZIP does compare different rendering policies. No supported fix is to silently restore the old white endpoint.
* The UI has no fixed named green/amber/blue phosphor catalog. The shared phosphor picker takes its 16 shortcut chips from the current ANSI color scheme. A different effective scheme/profile can display different chip values even while the independent phosphor default remains `#00FF41`. This is a code fact, not evidence that the user's profile changed.

## Portable copy and settings limits

The known copy `D:/nemo-workspace/Backslash-Dev-0.0.1.0-CRT-x64-8a1fbd77` exists and has an older/different renderer. A read-only filtered inspection found `Dark+`, glow 0.25, scanline intensity 0.85 and opacity 84 in its profile defaults, with no saved phosphor tint key. Settings/state hashes were unchanged across the read. This old directory is **not established as either member of the new comparison**, and no settings fault or user action is inferred from it. No active Backslash process was identified by the process queries.

`FileUtils.cpp` resolves portable settings beside the model module as `settings/`, when `.portable` is present. A fresh extracted folder therefore has its own settings; it does not share or automatically migrate another portable copy's overrides. All delivered copies use version 0.0.1.0, so that version alone cannot establish build identity. Use the ZIP/folder suffix and module fingerprint. No user's file was copied, reset or edited during this audit.

## Verification and narrow next step

Fresh `Test-RetroShader.ps1 -Runtime` passed 2,000 RGB/strength/alpha cases, ten white endpoints, 170 monotonic gray levels, combined CRT/DPI and ABI checks. The actual Atlas/BackendD3D `BackslashPhosphorOpacityWARP` content probe passed **1/1**. Those checks use owned offscreen WARP fixtures and do not prove native hardware/screenshot appearance.

No post-acceptance production regression is demonstrated, so **no phosphor shader/color repair is applied**. To isolate the reported difference, the missing evidence is: the exact correct/wrong ZIP or executable-folder names; whether the change is in picker chips, saved RGB values or rendered output; the selected profile and focused/unfocused appearance; and its tint/CRT/opacity/custom-shader values. A side-by-side screenshot would help distinguish UI-palette and rendered-color changes, but the two build identities plus effective values are the minimum deterministic comparison. Do not replace a running copy or reset settings to test this.

Frozen backups, requested independently of current code: [phosphor reference](PHOSPHOR-COLOR-REFERENCE-dc027cae.md) and [bezel palette reference](BEZEL-COLOR-REFERENCE-dc027cae.md), with exact JSON, source/blob hashes and SVG swatches. These files stay pinned to dc027cae when future defaults or palettes change.

Detailed evidence: `artifacts/phosphor-regression-audit-20261015-960a9f9/source-and-package-comparison.json`, `exact-phosphor-bytecode-match.json`, filtered `known-copy-settings-and-binaries.json`, `frozen-source-manifest.json`, `bezel-colors-derived.json`, shader/runtime logs and actual content-probe log. No commit, push, install, registration or running-installation overwrite was performed.
