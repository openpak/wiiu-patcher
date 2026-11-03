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

The server addresses are defined once, in `common/inkay_config.h` (`OPENPAK_SERVER_IP`,
`OPENPAK_NNCS2_IP`, `OPENPAK_IOSU_PORT`). Override them at build time:

```shell
docker run --rm -v ${PWD}:/app -w /app builder make OPENPAK_SERVER_IP=1.2.3.4 OPENPAK_NNCS2_IP=1.2.3.5 OPENPAK_IOSU_PORT=4443
```

The IOSU URLs use the address directly, so a server move needs a new release; see
[protocol](protocol.md#trade-off-a-server-move-is-a-release). The box needs TCP
`OPENPAK_IOSU_PORT` open and the `wiiu-system` route (protocol.md).

## Install on the console

1. Copy `Inkay-openpak.wms` to `sd:/wiiu/environments/aroma/modules/` and `Inkay-openpak.wps` to
   `sd:/wiiu/environments/aroma/plugins/`.
2. Power the console off and on.
3. Remove the files to go back to Nintendo's servers.

Aroma must be the release version configured for autoboot/coldboot. The upstream requirements
are in [provenance.md](provenance.md).

## Release

Pushing a `v*.*.*` tag runs `.github/workflows/release.yml`. It builds in the toolchain image
and creates a GitHub release with `Inkay-openpak.wms`, `Inkay-openpak.wps` and `data/ca.pem` attached, plus generated
notes. Other tag patterns do not trigger a release. `workflow_dispatch` builds without publishing.
