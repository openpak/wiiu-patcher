# Deploy: wiiu-patcher

## Build

The toolchain is the `Dockerfile` (devkitPPC plus the wiiu-env library images, pinned by tag).
CI runs the same two commands (`.github/workflows/ci.yml`, `release.yml`):

```shell
docker build . -t builder
docker run --rm -v ${PWD}:/app -w /app builder
```

The output is `Inkay-openpak.wms` in the repo root (`TARGET` in the `Makefile`). `make` also builds the app (`app/`, a `.wuhb` that embeds the module and plugin) and copies all three into `dist/`.

The server addresses are defined once, in `common/inkay_config.h` (`OPENPAK_SERVER_IP`,
`OPENPAK_NNCS2_IP`, `OPENPAK_IOSU_PORT`). Override them at build time:

```shell
docker run --rm -v ${PWD}:/app -w /app builder make OPENPAK_SERVER_IP=1.2.3.4 OPENPAK_NNCS2_IP=1.2.3.5 OPENPAK_IOSU_PORT=4443
```

The IOSU URLs use the address directly, so a server move needs a new release; see
[protocol](protocol.md#trade-off-a-server-move-is-a-release). The box needs TCP
`OPENPAK_IOSU_PORT` open and the `wiiu-system` route (protocol.md).

## Install on the console

1. Put `OpenPak-Wii-U-Patcher.wuhb` in `sd:/wiiu/apps/`.
2. Start **OpenPak Wii U Patcher** from the Wii U Menu. The status pill reads `NINTENDO` or `OPENPAK`; the card marks the live network `Active`.
3. Select **OpenPak** (A). The app copies `Inkay-openpak.wms` to `sd:/wiiu/environments/aroma/modules/` and `Inkay-openpak.wps` to `.../plugins/`, then asks `OpenPak selected. Reboot to apply?`. **A** reboots, **B** keeps the app open.
4. To return to Nintendo: select **Nintendo**, reboot.

Aroma must be the release version configured for autoboot/coldboot. The upstream requirements are in [provenance.md](provenance.md). The module loads at boot, so the console must reboot (not just return to the menu) after a change; if the app cannot reboot the console it relaunches the menu, and a power off and on does the rest.

### What is backed up

OpenPak's files live in `sd:/wiiu/openpak/system` as `<name>.managed`. If a file of yours sits where OpenPak installs, it is copied to `<name>.original` first (an older `Inkay-openpak` file from the manual route is kept as `<name>.replaced`, because it is an older OpenPak build). Selecting Nintendo restores those exactly and deletes what OpenPak added. A file changed by somebody else afterwards is kept and the app says `System files changed; originals kept in /wiiu/openpak/system`. If the Inkay settings file (`plugins/config/inkay.json`) had `connect_to_network` off, selecting OpenPak turns it on and selecting Nintendo turns it off again. If Pretendo's own Inkay files are present the app names them: two Inkays cannot run together.

`tests/run.sh` checks these rules on the host.

## Appendix: manual route (developers)

Copy `Inkay-openpak.wms` to `sd:/wiiu/environments/aroma/modules/` and `Inkay-openpak.wps` to `sd:/wiiu/environments/aroma/plugins/` (the release has both), power the console off and on. Remove the files to go back. The app recognises files installed this way (see above).

## Release

Pushing a `v*.*.*` tag runs `.github/workflows/release.yml`. It builds in the toolchain image
and creates a GitHub release with `OpenPak-Wii-U-Patcher.wuhb`, `Inkay-openpak.wms`, `Inkay-openpak.wps` and `data/ca.pem` attached, plus generated
notes. Other tag patterns do not trigger a release. `workflow_dispatch` builds without publishing.
