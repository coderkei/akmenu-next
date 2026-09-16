# Themes & Covers

Themes are directories under `_nds/akmenunext/ui`. AKMenu-Next supports Acekard/AKAIO, Wood R4 themes and it's own upgraded themes. The selected theme's directory supplies its artwork and settings. An installable theme must have its bitmap assets and a `uisettings.ini` containing `[global settings]`. `custom.ini` is optional.

Theme bitmap files must be 16-bit BMPs in X1 R5 G5 B5 (RGB 5:5:5) format. In GIMP, choose “16 bit (X1 R5 G5 B5)” in the BMP export options.

## Theme configuration files

Both files use INI format: a section name in square brackets followed by `key = value` lines. `uisettings.ini` controls the interface colors, positions, sizes, and theme features such as covers. `custom.ini` controls optional upper-screen additions such as the DS username, date, custom text, and custom picture. Edit the files inside the folder for the theme you have selected; omitted settings use built-in defaults. Positions and sizes are in screen pixels, and color values use packed DS 15-bit format (for example, `0x7fff` is white).

For example, from Blue Skies' `uisettings.ini` sets interface colors, the file-list colors, and the clock position and visibility:

```ini
[global settings]
formBodyColor = 0x5e06
formTextColor = 0x7fff
spinBoxNormalColor = 0x7fff
spinBoxTextColor = 0x4d80
spinBoxFrameColor = 0x35ad

[main list]
textColor = 0x2d6b
selectionBarColor1 = 0x20e2
selectionBarColor2 = 0x1482
selectionBarOpacity = 100

[big clock]
x = 6
y = 94
show = 1
```

The section tells AKMenu which feature a value belongs to. For instance, `x` and `y` under `[big clock]` position the clock, while `show = 1` displays it; setting `show = 0` hides it. A key only applies within its section: `x` and `y` under `[date]`, for example, position the date instead.

Blue Skies' `custom.ini` shows how an overlay can be positioned, colored, formatted, and switched on or off. In the shipped theme the date is disabled with `show = 0`; change it to `1` to display it:

```ini
[date]
x = 8
y = 148
color = 0x2d6b
show = 0
format = DD/MM/YYYY
showTime = 1
font = 0
```

## Standard text (`custom.ini`)

The standard text layer draws text on top of the theme's upper-screen background. All coordinates are screen coordinates. Colors use the DS 15-bit color format, such as `0x7fff`; `show` values are `0` or `1`.

### DS username

The `[user name]` section displays the username stored in the DS system settings:

```ini
[user name]
x = 60
y = 173
color = 0x7fff
show = 1
```

### Date and time

The separate `[date]` section displays the current DS system date using standard text by default. With `font = 0`, it does not require the calendar number graphics, so it can be used when a theme hides the calendar.

```ini
[date]
x = 8
y = 148
color = 0x7fff
show = 1
format = DD/MM/YYYY
showTime = 1
font = 0
```

Supported `format` values are:

| Value | Example |
| --- | --- |
| `DD/MM/YYYY` | `09/11/2026` |
| `MM/DD/YYYY` | `11/09/2026` |
| `YYYY/MM/DD` | `2026/11/09` |

Set `showTime = 0` to omit the time. The time follows the user's **12 hour** interface setting, displaying either `23:45` or `11:45 PM`.

Set `font = 1` to draw the date with digit sprites from `calendar/date_numbers.bmp` instead of standard text. The bitmap must be 9 pixels wide by 140 pixels high, with ten 9-by-14 digit sprites stacked vertically in order from `0` to `9`. The selected `format` still controls the day, month, and year order; slashes are omitted and a 9-pixel gap separates each field, leaving room for a slash in the theme background. Bitmap dates do not display the time, even when `showTime = 1`. The default `font = 0` keeps standard text rendering.

### Other standard text and picture options

The `[custom text]` section accepts `text`, `x`, `y`, `w`, `h`, `color`, and `show`. The `[custom picture]` section accepts `file`, `x`, `y`, and `show`.

## Clock and calendar (`uisettings.ini`)

The `[big clock]` section accepts `x`, `y`, and `show`. The optional `[am pm]` section accepts `x`, `y`, `color`, and `show` for the AM/PM label when 12-hour mode is enabled.

The calendar uses separate sections for its components:

* `[calendar year]`: `x`, `y`, `show`
* `[calendar month]`: `x`, `y`, `show`
* `[calendar dayx]`: `x`, `y`, `show` for a compact day number
* `[calendar day]`: `x`, `y`, `dw`, `dh`, `highlightColor`, `show` for the month grid

Set the relevant `show` values to `0` when a theme removes the calendar or needs that space clear. The `[date]` option in `custom.ini` can then provide a date overlay instead.

## UI colors and scroll indicators (`uisettings.ini`)

Color keys in the `[global settings]` section use packed DS 15-bit colors (for example, `0x7fff` for white). These keys control the scroll indicators shown in scrollable settings, theme lists, Help, and About windows:

| Key | Scroll indicator use |
| --- | --- |
| `spinBoxNormalColor` | Chevron for a direction that cannot scroll farther |
| `spinBoxTextColor` | Chevron for a direction that can scroll; colors the scrollbar thumb where a scrollbar is shown |
| `spinBoxFrameColor` | Outline of the scrollbar beside the chevrons |

For example:

```ini
[global settings]
spinBoxNormalColor = 0x001f
spinBoxTextColor = 0x7fff
spinBoxFrameColor = 0x4210
```

## Covers

AKMenu-Next displays a BMP cover on the top screen for `.nds`, `.dsi`, `.srl`, and `.ids` ROMs. Put `.bmp` covers at the filesystem root in `_nds/covers_code/` or `_nds/covers_name/`. It first looks for the four-character game code, then for the actual ROM filename without its final extension.

BMPs with four or more entirely black or transparent columns at the end is clipped off, (the covers from Pico-Cover seem to have this). A theme must include a `[cover]` section in `uisettings.ini` to opt in to cover display:

```ini
[cover]
x = 140
y = 44
darken = 0
fade = 0
```

`x` and `y` position the image; omitting either axis centres the cover on that axis. `darken` dims the complete top screen behind a loaded cover (`0` normal, `100` black), and `fade` makes the cover transparent (`0` opaque, `100` invisible). Both effects default to `0`. Themes without a `[cover]` section never display covers. The **Interface settings → Game covers** option overrides whether covers are displayed or not for themes that support them.

## Theme music (`bgm.bcstm` / `bgm.wav`)

Place a `bgm.bcstm` or `bgm.wav` in the theme directory, alongside its theme images (for example, `_nds/akmenunext/ui/blue skies/bgm.bcstm`). The music will play in the menu on a loop. **Interface settings → Theme music** can enable or disable playback. If both files exist, `bgm.bcstm` is used; if it cannot be played, `bgm.wav` is used instead.

`bgm.bcstm` is a 3DS music file: DSP-ADPCM, mono or stereo, up to 48,000 Hz. Loop points saved in the file are used, so the music can repeat past an intro instead of restarting from the beginning. Wii U `.bfstm` files are not supported. You can create these files with tools such as VGAudio or LoopingAudioConverter. The DS mixes its sound output at 32.768 kHz, so 32,000 Hz keeps the file small without losing quality.

`bgm.wav` must be uncompressed RIFF/WAVE PCM, 16-bit signed, mono or stereo (22,050 Hz mono keeps files small). It always repeats from the beginning. You can use tools such as Audacity & FFMPEG to export audio in this format.
