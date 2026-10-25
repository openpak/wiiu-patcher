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
- Resolves every `*.openpak.org` name to the OpenPak box itself; no DNS setup or public DNS record is needed.

## Install

1. Copy `Inkay-openpak.wms` to `sd:/wiiu/environments/aroma/modules/`.
2. Remove the file to go back to Nintendo's servers.

## Build

```shell
docker build . -t builder
docker run --rm -v ${PWD}:/app -w /app builder
```

See [deploy](docs/deploy.md).

## Docs

- [Protocol](docs/protocol.md)
- [Evidence](docs/evidence.md)
- [Deploy](docs/deploy.md)
- [Notes](docs/notes.md)
- [Upstream README](docs/upstream.md)
- [Docs index](docs/README.md)

## Licence

See [LICENSE](LICENSE).
