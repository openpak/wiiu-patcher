# Wii U Patcher

OpenPak's Wii U Patcher.

| | |
| --- | --- |
| Console | Wii U |
| Kind | HOME Menu app (`OpenPak-Wii-U-Patcher.wuhb`) that installs the Aroma module (`Inkay-openpak.wms`) and plugin (`Inkay-openpak.wps`) |
| Download | [Releases](https://github.com/openpak/wiiu-patcher/releases) (`vX.Y.Z` tags) |
| Based on | [Pretendo Inkay](https://github.com/PretendoNetwork/Inkay) (GPL-3.0, as upstream) |
| Licence | GPL-3.0, see [LICENSE](LICENSE) |

## What it does

- Switches an Aroma Wii U between Nintendo's servers and OpenPak's, and back: pick **Nintendo** or **OpenPak**, then reboot.
- Carries the OpenPak module and plugin inside itself and installs them into `sd:/wiiu/environments/aroma`. Anything it replaces is kept in `sd:/wiiu/openpak/system` (`.original` / `.absent`), and selecting Nintendo puts the exact original back and removes what it added.
- Rewrites every `*.pretendo.cc` name to the same-length `*.openpak.org` name and ships the OpenPak Network CA, so the console trusts the certificates served for those names.
- Resolves every `*.openpak.org` name to the OpenPak box itself on the PPC side, and points the IOSU services (account, eShop/SOAP, SpotPass, update list) at the box's IP address, so the console reaches OpenPak on automatic DNS with no DNS setup. The addresses are fixed at build time: a server move needs a new release (see [protocol](docs/protocol.md)).

## Install

1. Put `OpenPak-Wii-U-Patcher.wuhb` from the [release](https://github.com/openpak/wiiu-patcher/releases) in `sd:/wiiu/apps/`. It then shows on the Wii U Menu like any other app. (A Homebrew App Store entry is planned.)
2. Start **OpenPak Wii U Patcher** and select **OpenPak**. `OpenPak selected. Reboot to apply?` appears: **A** reboots now, **B** later.
3. To go back: start the app, select **Nintendo** and reboot.

Needs Aroma set up for coldboot (the app says so when Aroma is missing). A newer app version updates the module and plugin the first time it starts and offers the reboot. Full steps and the manual route are in [deploy](docs/deploy.md).

## Build

```shell
docker build . -t builder
docker run --rm -v ${PWD}:/app -w /app builder
```

See [deploy](docs/deploy.md).

## Docs

- [Protocol](docs/protocol.md)
- [Deploy](docs/deploy.md)
- [Provenance](docs/provenance.md)

## Licence

See [LICENSE](LICENSE).

PRDs: openpak/prds (private)
