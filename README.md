# akmenu-next

Frontend for nds-bootstrap based upon lifehansolhacker's akmenu4 port [https://github.com/lifehackerhansol/akmenu4](https://github.com/lifehackerhansol/akmenu4).

## Getting Started

Requires devkitarm (pre-calico) to be installed with the `nds-dev` package. Simply build the repository with `make`.


### Configuration

* The system directory is `_nds/akmenunext` where the akmenu system files should be placed, along with a copy of nds-bootstrap in the `_nds` folder as `nds-bootstrap-release.nds`.
* The binary of akmenu-next additionally needs to be in the `_nds/akmenunext` folder as `launcher.nds` for theme & language reboots.
* Cheats should be placed as `usrcheat.dat` into the `_nds/akmenunext/cheats` folder.
* Themes go into `_nds/akmenunext/ui`. Acekard & Wood R4 themes are supported.

## License

This project is comprised of various sources and is collectively licensed under the GPL-3.0-or-later license.
Please check [the license section](https://github.com/coderkei/akmenu-next/tree/main/licenses) for more information as well as a copy of all applicable licenses.
