# Appended bezel finishes

These two finishes append stable enum values; the original nine indices, RGB values, gradient directions and blend percentages are unchanged.

| Finish | JSON token | Enum | Base | Casing top | Casing bottom | Rim top | Rim bottom | Recess top | Recess bottom |
| --- | --- | ---: | --- | --- | --- | --- | --- | --- | --- |
| Muted retro red | `mutedRed` | 9 | `#7C5955` | `#836460` | `#6A4C49` | `#977F79` | `#44302E` | `#271C1B` | `#836762` |
| Warm mustard | `warmMustard` | 10 | `#95814F` | `#99885B` | `#806E43` | `#A99C75` | `#51462B` | `#2F2919` | `#98885D` |

Both use the existing opaque dark-finish material branch, like slate blue and muted olive: casing highlight mixes 10% neutral #C5CCC7, casing shadow 14% black; rim highlight 27% #E2E6DD, rim shadow 45% black; recess shadow 68% black and highlight 16% #A9B1A9. Integer blend rounding is unchanged. Muted red is an earthy warm red and mustard is a subdued golden yellow. No rendering matrix or visual test was run for this build.

These finishes are independent of the [CurrentColor badge part controls](CURRENTCOLOR-BADGE-CONTROLS.md). No existing finish, source artwork, window geometry or terminal color is altered.
