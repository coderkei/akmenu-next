<p align="center">
   <img width="802" height="211" alt="image" src="https://github.com/user-attachments/assets/93b07874-5f3f-4572-b2e3-74bdabd757a0" />
</p>

<p align="center">
   <a href="https://gbatemp.net/threads/ds-i-3ds-akmenu-next-wood-frontend-for-nds-bootstrap.665743/">
      <img src="https://img.shields.io/badge/GBAtemp-Thread-blue.svg" alt="GBAtemp thread">
   </a>
   <a href="https://github.com/coderkei/akmenu-next/actions/workflows/main.yml">
      <img src="https://github.com/coderkei/akmenu-next/actions/workflows/main.yml/badge.svg" alt="Build status on GitHub Actions">
   </a>
</p>

## AKMenu-Next

AKMenu-Next is a frontend for [nds-bootstrap](https://github.com/DS-Homebrew/nds-bootstrap/) and [Pico-Loader](https://github.com/LNH-team/pico-loader), based on [lifehansolhacker's akmenu4 port](https://github.com/akmenu4/akmenu4). The goal is to bring back the classic Acekard/Wood menu on modern setups, and to extend and modernise it along the way. It runs on flashcarts, the DSpico, DSi and 3DS.

Features:

* Launch DS games and homebrew with nds-bootstrap or Pico-Loader, with per-game settings
* Themes, game covers and theme music
* Cheats with `usrcheat.dat`
* Plugins for other file types (NES, SNES, GBC, GBA, Videos and more)
* Save slots for DS games
* Slot-1 cards on DSi (with the card's banner shown in the menu), Slot-2 and EZ Flash 3 in 1 support for GBA games
* Available in multiple languages

## Installing

Check the [AKMenu-Next Docs](https://coderkei.github.io/akmenu-next-docs/) for setting up or updating AKMenu-Next.

## Making themes

You can make your own themes with [AKTheme Studio](https://coderkei.github.io/akmenu-next-theme-editor/). The theme files, covers and music are documented in [Themes & Covers.md](Themes%20%26%20Covers.md).

## Building from source

AKMenu-Next builds with BlocksDS 1.25.0 on the Wonderful Toolchain. Follow the BlocksDS setup guide for [Windows](https://blocksds.skylyrac.net/docs/setup/windows/) or [Linux](https://blocksds.skylyrac.net/docs/setup/linux/) to get `blocksds-toolchain` installed. You'll also need `make` and `zip`.

Clone the repo with submodules (`nds-bootloader` is needed) and keep the folder named `akmenu-next`, since the CIA gets its name from the folder and the package script looks for `akmenu-next.cia`.

Then from the BlocksDS shell (the Wonderful Toolchain Shell on Windows):

```sh
make -j2 && cd package && ./package.sh
```

This gives you `akmenu-next-flashcart.zip`, `akmenu-next-pico.zip`, `akmenu-next-dsi.zip` and `akmenu-next-3ds.zip` in the `package` folder.

The [GitHub Actions build](.github/workflows/main.yml) uses the [BlocksDS Docker image](https://hub.docker.com/r/skylyrac/blocksds) if you'd rather build that way.

### Configuration

* The system folder is `_nds/akmenunext`. The AKMenu-Next system files go there, and the nds-bootstrap files go in `_nds`.
* AKMenu-Next also needs to be in `_nds/akmenunext` as `launcher.nds` so it can reboot for theme and language changes. The package script adds this for you.
* Cheats go in `_nds/akmenunext/cheats` as `usrcheat.dat`.
* For flashcart builds, the Pico-Loader files for your flashcart go in the `_pico` folder.
* Plugins go in `_nds/akmenunext/plugins`.

### Plugins

A plugin is an ini file named after the extension it handles, e.g. `gba.ini`:

```
[plugin]
path=path/to/default.nds // Optional fallback path
dsipath=path/to/dsi.nds // Optional override when DSi/TWL mode is detected
ntrpath=path/to/ds.nds // Optional override when DS/NTR mode is detected
argv=1 //If the emulator supports argv
bootstrap=0 //Use the Homebrew Loader setting
hbargv=0 //Pass the plugin argument through nds-bootstrap-hb's HOMEBREW_ARG setting
```

Files with that extension will show up in the file list and open with the `.nds` you set. You need at least one of `path`, `dsipath` or `ntrpath`. `dsipath` and `ntrpath` are used in DSi and DS mode, and `path` is the fallback if the one for the current mode isn't set. If there's no path for the current mode the plugin won't launch.

* `argv=1` passes the file path to the emulator as a launch argument.
* `bootstrap=1` uses nds-bootstrap-hb for the plugin on DSi when there's no flashcart. Otherwise the Homebrew Loader setting is used. If you turn on **Override plugin defaults** in Other Settings, the Homebrew Loader setting is always used, even with `bootstrap=1`.
* `hbargv=1` writes the file path to `HOMEBREW_ARG` in `nds-bootstrap.ini` when nds-bootstrap-hb is the Homebrew Loader. This is separate from `argv`.

The icon comes from `_nds/akmenunext/plugins/icons/{extension}.bin` (a banner file). If that's missing, the default icon is used. You can also put a banner next to a file, e.g. `Game.gbc.bin`, to give that file its own icon and title.

If you make a `.gba` plugin and have an EZ Flash 3 in 1 inserted, the 3 in 1 is used for `.gba` files instead.

There's more in the [AKMenu-Next Plugin Documentation](https://coderkei.github.io/akmenu-next-docs/guides/plugins/).

## License

This project is made up of code from a few different sources and is licensed as a whole under GPL-3.0-or-later.
See [the licenses folder](https://github.com/coderkei/akmenu-next/tree/main/licenses) for more info and copies of all the licenses that apply.

## Contributing

Contributions are welcome, just make sure you've read [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request. Game compatibility issues should go to [nds-bootstrap](https://github.com/DS-Homebrew/nds-bootstrap) or [Pico-Loader](https://github.com/LNH-team/pico-loader) instead.

![image](https://www.gnu.org/graphics/gplv3-127x51.png)
