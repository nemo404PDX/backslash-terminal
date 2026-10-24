# Black and White scanlines

In a profile's Appearance settings, enable Retro terminal effects, then choose
**Scanline color → Black or White**. Black is the unchanged default. Both use the
same intensity, thickness and gap controls. Default intensity remains 10%; White
at that strength gives light bands on dark backgrounds, not a bright white screen.
Full White intensity reaches white in fully covered line bands. Gaps are unchanged.
A custom pixel shader still takes precedence.

JSON appearance key: `experimental.retroTerminalScanlineWhite` (boolean).
`true` selects White; `false` selects Black. Omit/null resets to the inherited
choice, with Black as the root default. The value is supported in profile defaults,
profiles and unfocused appearance, and survives setup export/import without
flattening inherited choices. Reset in the UI clears the local override.

White bands follow phosphor tint so the selected phosphor color is not inverted
or recolored in gaps. They preserve the coverage alpha after existing glow;
fully transparent pixels remain transparent and will not get painted-over bands.
The black path retains its existing multiplication and alpha behavior exactly.
This choice does not change the phosphor mapping, add a contrast guard or expand
the Settings preview sample. Very strong bands can obscure text; use intensity
for the preferred subtle effect.

Isolated WARP tests cover true black, colored backgrounds, foreground tones,
partial/zero alpha, fractional display scaling, gap/thickness, tint ordering,
zero/full/subtle intensity and legacy black results. GPU-readback comparisons
show neutral, Black 10%, White 10%, White 50%, and White 10% with magenta phosphor.