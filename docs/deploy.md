# Deploy: wiiu-patcher

## Build

The toolchain is the `Dockerfile` (devkitPPC plus the wiiu-env library images, pinned by tag).
CI runs the same two commands (`.github/workflows/ci.yml`, `release.yml`):

```shell
docker build . -t builder
docker run --rm -v ${PWD}:/app -w /app builder
```

The output is `Inkay-openpak.wms` in the repo root (`TARGET` in the `Makefile`). `make` also
copies it and `plugin/*.wps` into `dist/`.

The server addresses are defined in `common/inkay_config.h` (`OPENPAK_SERVER_IP`,
`OPENPAK_NNCS2_IP`). Override the first at build time with `-DOPENPAK_SERVER_IP='"<address>"'`.

## Install on the console

1. Copy `Inkay-openpak.wms` to `sd:/wiiu/environments/aroma/modules/`.
2. Remove the file to go back to Nintendo's servers.

Aroma must be the release version configured for autoboot/coldboot. The upstream requirements
are in [upstream.md](upstream.md).

## Release

Pushing a `v*.*.*` tag runs `.github/workflows/release.yml`. It builds in the toolchain image
and creates a GitHub release with `Inkay-openpak.wms` and `data/ca.pem` attached, plus generated
notes. Other tag patterns do not trigger a release. `workflow_dispatch` builds without publishing.
