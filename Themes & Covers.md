# Themes & Covers

Themes are directories placed in `_nds/akmenunext/ui`. AKMenu-Next supports Acekard/AKAIO and Wood R4 themes. A theme directory contains its bitmap assets plus `uisettings.ini` and `custom.ini`.

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

The separate `[date]` section displays the current DS system date using standard text. It does not require the calendar number graphics, so it can be used when a theme hides the calendar.

```ini
[date]
x = 8
y = 148
color = 0x7fff
show = 1
format = DD/MM/YYYY
showTime = 1
```

Supported `format` values are:

| Value | Example |
| --- | --- |
| `DD/MM/YYYY` | `09/11/2026` |
| `MM/DD/YYYY` | `11/09/2026` |
| `YYYY/MM/DD` | `2026/11/09` |

Set `showTime = 0` to omit the time. The time follows the user's **12 hour** interface setting, displaying either `23:45` or `11:45 PM`.

### Other standard text and picture options

The `[custom text]` section accepts `text`, `x`, `y`, `w`, `h`, `color`, and `show`. The `[custom picture]` section accepts `file`, `x`, `y`, and `show`.

## Clock and calendar (`uisettings.ini`)

The `[big clock]` section accepts `x`, `y`, and `show`. The optional `[am pm]` section accepts `x`, `y`, `color`, and `show` for the AM/PM label when 12-hour mode is enabled.

The calendar uses separate sections for its components:

* `[calendar year]`: `x`, `y`, `show`
* `[calendar month]`: `x`, `y`, `show`
* `[calendar dayx]`: `x`, `y`, `show` for a compact day number
* `[calendar day]`: `x`, `y`, `dw`, `dh`, `highlightColor`, `show` for the month grid

Set the relevant `show` values to `0` when a theme removes the calendar or needs that space clear. The `[date]` option in `custom.ini` can then provide a text date instead.

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
