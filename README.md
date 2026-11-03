# Wii U Patcher

OpenPak's Wii U Patcher.

| | |
| --- | --- |
| Console | Wii U |
| Kind | Aroma module (`Inkay-openpak.wms`) |
| Download | [Releases](https://github.com/openpak/wiiu-patcher/releases) (`vX.Y.Z` tags) |
| Based on | [Pretendo Inkay](https://github.com/PretendoNetwork/Inkay) (GPL-3.0, as upstream) |
| Licence | GPL-3.0, see [LICENSE](LICENSE) |

## What it does

- Rewrites every `*.pretendo.cc` name to the same-length `*.openpak.org` name.
- Ships the OpenPak Network CA (`data/ca.pem`) so the console trusts the certificates served for those names.
- Resolves every `*.openpak.org` name to the OpenPak box itself on the PPC side, and points the IOSU
  services (account, eShop/SOAP, SpotPass, update list) at the box's IP address, so the console
  reaches OpenPak on automatic DNS with no DNS setup and no public DNS record. The addresses are
  fixed at build time: a server move needs a new release (see [protocol](docs/protocol.md)).

## Install

1. Copy `Inkay-openpak.wms` to `sd:/wiiu/environments/aroma/modules/` and `Inkay-openpak.wps` to
   `sd:/wiiu/environments/aroma/plugins/`.
2. Power the console off and on (IOSU patches load at boot).
3. Remove the files to go back to Nintendo's servers.

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
