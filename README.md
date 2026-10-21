# Backslash Terminal

Backslash is a Windows-focused fork of Microsoft Windows Terminal, adding profile tools and optional CRT styling to its tabs, panes and terminal foundation.

**Windows is the current target. Linux and macOS support are TBD.** Current deliveries are portable development builds.

## Implemented in development builds

- **Portable isolation:** adjacent settings/state and separate Backslash local storage, with automatic upstream settings import and external fragment discovery disabled. See [portable implementation](docs/backslash/PORTABLE-IMPLEMENTATION.md).
- **CRT controls:** saved per-profile glow and scanline strengths with settings UI and inheritance/reset support. See [CRT appearance](docs/backslash/CRT-APPEARANCE.md).
- **Retro bezel:** size/color presets, optional top/bottom badges, fullscreen aperture centering and a separate opaque surround around the terminal opening. See [bezel implementation and design](docs/backslash/BEZEL-DESIGN.md).
- **SSH profiles and WSL:** remote shell choices, explicit SSH host scan/add/restore with stable profile identities, and modern WSL distro discovery. Saved shell choices apply to new SSH sessions. See [SSH/WSL behavior and evidence](docs/backslash/SSH-PROFILES.md).

Automated model, shader and portable startup checks are recorded in those documents. Native visual/interaction checks and live-server verification of the latest SSH fixes remain outstanding.

## Planned additions

- Named saved workspaces, tab grouping and richer SSH bookmarks.
- Palette copy/fill tools, a curated font catalog, and improved inline graphics and command editing/history.
- Integrated file navigation/transfers and further CRT, material and glass effects.

These are design goals, not shipped features or a release schedule. See the [roadmap](docs/backslash/ROADMAP.md), [feature scope](docs/backslash/FEATURES.md) and [file-transfer design](docs/backslash/FILE-TRANSFER-DESIGN.md).

## Build and documentation

Start with the [Windows build guide](tools/backslash/README.md), then the [portable build/run instructions](docs/backslash/PORTABLE-IMPLEMENTATION.md). The [project documentation index](docs/backslash/README.md) links the detailed designs and validation records.

## Screenshots

#### Example profile customization combinations:

<div style="text-align: center;">
<br/><img src="docs/images/backslash-window-01.png" alt="" width="620" ><br/>
<br/><em>A Backslash terminal window running Hermes Agent, customized with bezel size and colors, scanline size and separation, and screen phosphor color.</em>
</div><br/><br/>

<div style="text-align: center;">
<br/><img src="docs/images/backslash-window-02.png" alt="" width="620" >
</div><br/>

<div style="text-align: center;">
<br/><img src="docs/images/backslash-window-03.png" alt="" width="620" >
</div><br/>

<div style="text-align: center;">
<br/><img src="docs/images/backslash-window-04.png" alt="" width="620" >
</div><br/>

<div style="text-align: center;">
<br/><img src="docs/images/backslash-window-05.png" alt="" width="620" >
</div><br/>

<div style="text-align: center;">
<br/><img src="docs/images/backslash-window-06.png" alt="" width="620" >
</div><br/>

<div style="text-align: center;">
<br/><img src="docs/images/backslash-window-07.png" alt="" width="620" >
</div><br/>

<div style="text-align: center;">
<br/><img src="docs/images/backslash-window-08.png" alt="" width="620" >
</div><br/>

>**DEV NOTE**:  Save future screenshots in [`docs/images`](docs/images/README.md). That folder's guide covers suggested captures and removing credentials/private details. Images will be added here after they are supplied and reviewed.

<br/>

## Attribution and licensing

Backslash derives from [Microsoft Windows Terminal](https://github.com/microsoft/terminal), including its console and shared components. The source baseline and preparation history are recorded in [upstream provenance](UPSTREAM-SOURCE.md).

Original Microsoft attribution, the [MIT license](LICENSE), [third-party notices](NOTICE.md) and component licenses are retained. See also the [contributing guide](CONTRIBUTING.md) and [code of conduct](CODE_OF_CONDUCT.md).
