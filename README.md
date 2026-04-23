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

Frontend for [nds-bootstrap](https://github.com/DS-Homebrew/nds-bootstrap/) and [Pico-Loader](https://github.com/LNH-team/pico-loader) based upon [lifehansolhacker's akmenu4 port](https://github.com/lifehackerhansol/akmenu4).

## Installing

Please check the [AKMenu-Next Docs](https://coderkei.github.io/akmenu-next-docs/) for setting up or updating AKMenu-Next.

## Building from source

Requires the pre-calico version of devkitarm to be installed with the `nds-dev` package.
A docker image is available with this version on [docker hub](https://hub.docker.com/layers/devkitpro/devkitarm/20241104).

Build the repository with `make`, then run `package.cmd/sh` depending on your OS to package the build for Flashcarts, DSi and 3DS.

### Configuration

* The system directory is `_nds/akmenunext` where the akmenu system files should be placed, along with a copy the nds-bootstrap files in the `_nds` folder.
* The binary of akmenu-next additionally needs to be in the `_nds/akmenunext` folder as `launcher.nds` for theme & language reboots, this is automatically added if using the package script.
* Cheats should be placed as `usrcheat.dat` into the `_nds/akmenunext/cheats` folder.
* Themes go into `_nds/akmenunext/ui`. Acekard & Wood R4 themes are supported.
* For flashcart related builds, files for the corresponding flashcart for Pico-Loader should go into the `_pico` folder.
* Plugins go into `_nds/akmenunext/plugins`.

### Plugins

Plugin format is `{extension}.ini` (e.g. `gba.ini`) and follows the below format:


```
[plugin]
path=path/to/nds/here.nds
argv=1
```

The specified extension will be recognised as a valid file and will launch via argv (if set to `1`) to the specified `.nds` file.
The icon for the file will be read from `_nds/akmenunext/plugins/icons/{extension}.bin` as a banner file. If it is not found it will fall back to the default icon.

If the `.gba` extension is used for a plugin and an EZ Flash 3 in 1 is inserted, the 3 in 1 will take priority over the plugin for loading `.gba` files.

## License

This project is comprised of various sources and is collectively licensed under the GPL-3.0-or-later license.
Please check [the license section](https://github.com/coderkei/akmenu-next/tree/main/licenses) for more information as well as a copy of all applicable licenses.

## Contributing

Contributions are welcome, any issues regarding game compatibility should submitted on the [nds-bootstrap](https://github.com/DS-Homebrew/nds-bootstrap) repository.

![image](https://www.gnu.org/graphics/gplv3-127x51.png)
