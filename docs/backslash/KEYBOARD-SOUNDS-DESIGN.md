# Keyboard sounds design

[Project entry point](README.md) | [Roadmap](ROADMAP.md) | [CRT appearance](CRT-APPEARANCE.md) | [Settings layout proposal](SETTINGS-LAYOUT-PROPOSAL.md)

Status: **future design, not implemented**. Documentation authorized on 2026-10-14; no sound implementation is approved. Groups and their current fixes remain the priority. This is a bounded optional retro/mechanical keyboard-audio feature for laptop/membrane keyboard users, not a keyboard editor.

## Agreed sound model

Audio represents coherent switch and keyboard-assembly mechanics, rather than arbitrary noises assigned to letters. Use related press/release recordings or synthesis with restrained variants that belong to the same setup. Key-group differences represent that setup's predefined mechanics.

Exactly three sound profiles:

| Profile | Behavior |
| --- | --- |
| Normal | Normal switch sound throughout. |
| Silent | Dampened switch sound throughout; **not mute**. |
| Mixed | Normal main typing keys; silent modifiers, F1-F12 and numeric keypad. |

There is no reverse Mixed profile. Disable/mute is separate from Silent.

For Mixed, define main typing keys as letters, number row, punctuation and Space/Enter/Backspace/Tab in the main typing block. Modifiers are left/right Shift, Ctrl, Alt/AltGr and Windows/Meta. F1-F12 form the function row. Numeric keypad includes its digits, decimal, operators and keypad Enter; distinguish keypad keys from number-row/main Enter even when virtual-key values overlap. These are classification definitions for the plan, not per-letter sound assignments. Caps Lock, Escape, arrows, Home/End/Page Up/Page Down/Insert/Delete navigation cluster, Num Lock, Scroll Lock, Print Screen/Pause, Fn/media/vendor keys and ambiguous compact-layout keys need a later classification decision. Do not infer user choices for those keys or add extra sound profiles.

## Switch style, Weight and Volume

Switch style is separate from Normal/Silent/Mixed. Candidate audio presets are **full-height mechanical**, **low-profile** and **magnetic keyboard setup**. These are candidate coherent recording/synthesis setups, not claims that actuation technology uniquely determines acoustic character; keycaps, case, plate, mounting, damping and the chosen assembly also matter.

One overall **Weight** control fine-tunes the predefined setup while preserving relative differences: heavier main keys, lighter pinky-related keys and predefined other groups. Exact key-group assignments and relative curves require design/listening decisions; no per-key spring/lubrication sliders or elaborate customization editor.

Weight should affect perceived stroke/return timing and character, not merely loudness. Explore matched sample sets, envelopes and other bounded transformations without promising physical force simulation. A physical spring's length alone does not determine timing. Define useful bounds through listening tests so press/release pairs remain coherent at all weights. Do not move terminal input timing to imitate mechanics.

**Volume** is separate from Weight. Per-terminal-profile opt-in and volume are parent suggestions, not selected settings scope or persisted keys. Clear disable/mute and accessible labels are required; previews and final defaults remain design choices. Audio must remain optional and never be the sole feedback channel. Respect system mute/output routing and avoid surprise playback during configuration.

## Input ownership and lifecycle

Pair audio to actual key-down/key-up events received by the focused eligible terminal. Track physical key identity sufficiently to pair down/up and distinguish keypad/modifier sides. Do not derive sound from resulting text, terminal bytes or arbitrary letter names; shortcuts and IME/TSF/synthetic events require an explicit eligibility policy.

A held key generates one physical press and one release; OS autorepeat must not fabricate repeated physical downstrokes. Overlapping keys need bounded polyphony and independent pairing. Focus loss, tab/window changes, unload and disable must clear held-key state and stop/finish voices safely; decide audible cleanup versus silent cancellation without inventing releases or leaking state into the next control.

**Paste remains silent**, with no per-character audio queue. A real physical shortcut may produce its own eligible key events, but pasted text generates no strokes. Incoming terminal output, including echoed text, never generates keyboard audio. Existing terminal bell behavior is independent; this requirement does not suppress an application-requested bell.

Never delay actual terminal input for audio, await playback in key handling, or capture keys globally. No global keylogging, persistent key-content recording or cloud dependency. Backend/sample choice and local caching are candidate implementation decisions.

## Verified source seams versus proposals

Source checked on 2026-10-14. These seams exist; no keyboard-audio code or settings are implied.

- [TermControl key handlers](../../src/cascadia/TerminalControl/TermControl.cpp#L1576) route down/up separately into _KeyHandler; the routed path reads scan code and extended-key status. Investigate repeat state and alternate/synthetic input paths before choosing one nonduplicating observation point.
- [TermControl focus loss](../../src/cascadia/TerminalControl/TermControl.cpp#L2445) delegates interactivity focus cleanup. Research audio state cleanup across terminal/tab/window lifetimes without intercepting shell input.
- [Clipboard entry](../../src/cascadia/TerminalControl/TermControl.cpp#L2621) and [ControlCore::PasteText](../../src/cascadia/TerminalControl/ControlCore.cpp#L1494) are distinct from physical key handling; preserve filtering/bracketed paste and silence.
- [ControlCore output handler](../../src/cascadia/TerminalControl/ControlCore.cpp#L2297) writes connection output into the terminal. This is not an audio trigger.
- [TerminalPaneContent bell playback](../../src/cascadia/TerminalApp/TerminalPaneContent.cpp#L283) uses profile bell resources or a system sound; its _playBellSound path creates a MediaPlayer and notes Windows N availability. It is a reference for failure handling, not proof that this backend supports keyboard latency/polyphony.
- [Existing BellSound model setting](../../src/cascadia/TerminalSettingsModel/MTSMSettings.h#L107) and [profile bell preview/editor](../../src/cascadia/TerminalSettingsEditor/Profiles_Advanced.cpp#L50) show settings/media seams. Keyboard sound needs separately decided keys, inheritance and Save/Reset behavior; do not overload bellSound or imply current UI support.

Cross-link with [appearance planning](CRT-APPEARANCE.md) and [settings navigation proposal](SETTINGS-LAYOUT-PROPOSAL.md) for discoverability. No UI manifest fields, localization resources or runtime settings are added by this document.

## Bounded future implementation slice

After separate authorization, start with one coherent local preset and the three agreed sound profiles, one Weight control, independent Volume and explicit enable/mute. Resolve unspecified key classifications and settings scope first. Prototype paired events and listening behavior before extending sample styles.

Parent candidates: original/licensed recordings or local synthesis, cached/preloaded assets, bounded sample memory/voices, low-latency nonblocking playback and no cloud dependency. Choose voice limits, overflow policy, latency target and backend after measurement; avoid an unbounded queue or disk access on each key event. Disabled mode should perform no sample loading, voice allocation, scheduling or playback work beyond a cheap eligibility check.

Record asset provenance, redistribution/license terms and attribution locally before packaging. No unlicensed sample import, third-party setup or package is selected. Missing backend/device/assets should leave typing intact and report a useful configuration failure without repeated dialogs.

## Acceptance and open decisions

Future checks, all **unrun**:

1. Normal/Silent/Mixed match the table; Silent stays audible/dampened, mute is separate, and no reverse Mixed appears. Verify number row versus keypad, modifier sides and the decided unspecified-key mapping.
2. Paired down/up timing follows physical events; held-key repeat produces no fake presses; overlap/polyphony remains coherent at all Weight values. Test lost release, focus switch, window unload and disable/re-enable without stuck voices.
3. Paste large/Unicode/multiline/bracketed payloads and verify zero per-character sound/queue. Incoming/echoed/streamed terminal output never triggers keyboard audio; bell remains independent.
4. Measure event-to-audio latency, typing throughput and resource bounds under fast overlap; audio introduces no terminal input delay. Test backend/device/sample failure without dropped or reordered input.
5. Disabled/muted path loads no samples and creates/schedules no voices; default/opt-in policy produces no unexpected sound. Verify accessible naming, keyboard control and system mute.
6. Listening tests assess paired coherence, restrained variants, style identity and relative Weight behavior, including heavier main/lighter pinky groups. Weight changes character/timing separately from Volume, within documented bounds.
7. Verify settings Save/Reset/reload and profile inheritance once scope is chosen; test no duplicate triggers through alternate key/IME/TSF paths. Audit that observation stays within focused app controls and stores no key content.
8. Validate sample licenses/notices, offline operation, bounded cache/voice cleanup and simultaneous terminal/window behavior.

Open: unspecified-key classification, exact predefined weight groups/curves/bounds, preset recordings/synthesis/backend, mute/default/settings scope, shortcut/IME eligibility, cleanup policy and measured latency/voice limits. None implies implementation approval.
