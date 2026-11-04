# Changelog — nn-inkay

Generated from git history on 2026-09-15. `git log` stays the source
of truth; this file is the readable summary.

## v0.6.1 — 2026-11-04

- **Update and shop hosts are never redirected.** The network-wide list contains `.nintendo.net` and `.nintendowifi.net`, which also cover the update CDNs. The never-set now holds `.shop.nintendo.net`, `.shop.nintendowifi.net`, `.cdn.nintendo.net` and `.cdn.nintendowifi.net`, so `tagaya-wup.cdn`, `nus.cdn.wup.shop`, `ccs.cdn.wup.shop`, the `c.shop` hosts and the rest of the update path stay with Nintendo. `tests/run.sh` proves each against the live ceiling v18. The eShop SOAP hosts Inkay rewrites by URL (ecs, ias, cas) are OpenPak names and stay on OpenPak.

## v0.6.0 — 2026-11-04

- **One signed redirect list for the whole network.** The module redirects a game-server name to OpenPak only when it is on the list: the bundled base (`*.openpak.org`, the two NAT check hosts) plus the network-wide signed list, the same Ed25519 envelope the Switch and the emulators verify (`/api/v1/network/ceiling`, pinned key, version never goes down). The list is the payload's `all` when present, otherwise every platform's families together; another console's names never match on a Wii U. Everything else resolves normally.
- The plugin loads the cached list from `sd:/wiiu/openpak/redirects.json` at boot and fetches a fresh one in the background once the network is up (CURLWrapperModule, public TLS). A bad signature, a missing cache or no network leaves the cached or bundled list.
- Never redirected, whatever the list says: tagaya and nus (game and system updates stay with Nintendo), `conntest.nintendowifi.net`, and the bare `openpak.org` where the list is fetched.
- The patcher app shows `Redirects: N names (signed list vX)`, or `(bundled list)`. Host test: `tests/run.sh`.

## v0.5.1 — 2026-11-04

- **Game and system updates come from Nintendo.** The tagaya version list and NetUpdate (nus.wup) URLs in IOS-NIM are no longer rewritten, so dumped games and system titles update straight from Nintendo's live servers. Account, friends, NEX, SpotPass, Miiverse and the ECS/IAS eShop paths stay on OpenPak.

## v0.5.0 — 2026-11-04

- **OpenPak Wii U Patcher**, a `.wuhb` app (`OpenPak-Wii-U-Patcher.wuhb`, in `app/`): one status screen with the
  Switch Patcher's flow and wording (OPENPAK / NINTENDO pill, Nintendo and OpenPak rows with `Active`,
  `A Select`, `B Exit`, `OpenPak selected. Reboot to apply?` with `A Reboot now` / `B Later`, `Failed: <reason>`).
- It carries the module and plugin and installs them into Aroma with backup and exact rollback in
  `sd:/wiiu/openpak/system`; a newer app updates them and offers the reboot. Files from the manual route are
  recognised.
- Releases attach the `.wuhb` next to the module and plugin. `tests/run.sh` checks the backup rules on the host.

## v0.4.1 — 2026-11-04

- Wording follows the Switch Patcher: the plugin is listed as "OpenPak Wii U Patcher" (was "Inkay"), the status toast says "Using Nintendo" and the restart item says "Reboot to apply"; the uwu locale no longer says Pretendo. The settings storage id stays `inkay`, so saved settings carry over.

## v0.4.0 — 2026-11-03

- IOSU services reach OpenPak with no DNS record: the act, nim and boss URL patches are IP
  literals (`https://<OPENPAK_SERVER_IP>:4443/...`, plain-HTTP ones on port 80) instead of
  `*.openpak.org` names, which IOSU resolved through the console's real DNS. The box routes the
  Wii U system entrypoint (TCP 4443) by path. A server move now needs a new release
- `OPENPAK_SERVER_IP`, `OPENPAK_NNCS2_IP`, `OPENPAK_IOSU_PORT` are the single source, settable
  as Makefile variables; a build fails if an address makes an IOSU string overflow its slot
- `n.app.openpak.org` stays a name (unmeasured, possibly a suffix)
- Releases attach `Inkay-openpak.wps` as well as the `.wms`

## v0.3.0 — 2026-11-01

- The plugin is `Inkay-openpak.wps` (was `Inkay-pretendo.wps`), so it can't be mistaken for
  Pretendo's plugin of the same name on the SD card

- ci: release builds on `v*.*.*` tags only (was `openpak-v*`), CI/CodeQL on manual dispatch,
  artifacts kept 3 days [d33e32e, 898e304]; docs commits

## openpak-v0.2.0 — 2026-09-10

- Resolve every `*.openpak.org` name on the console itself (`common/inkay_config.h`,
  `-DOPENPAK_SERVER_IP`): no public DNS record needed [23b207c]

## openpak-v0.1.1 — 2026-09-10

- The module file is `Inkay-openpak.wms` [9d1cfdd]

## openpak-v0.1.0 — 2026-09-10

- openpak.org names (same length as pretendo.cc), the OpenPak CA, release on `openpak-v*`
  tags [5c753b7]; `NETWORK_BASEURL` is openpak.org too [1bca437]

## v3.0.0 — 2025-02-15



## v2.6.0 — 2024-07-25



## v2.5.0 — 2024-05-28



## v2.4.0 — 2024-04-08



## v2.3.0 — 2023-08-11



## v2.2.1 — 2023-04-25



## v2.2.0 — 2023-04-09



## v2.1.0 — 2022-12-10



## v2.0.0 — 2022-08-31



## v2.0.0 — 2022-08-31

- Update README.md [8ef4711]
- ci: Add CI [004e28c]
- Add IOSU-side SSL and URL patches [24a1753]
- Update tools and libraries [d23f774]
- Remove WUPS v0.1 support [935e3bd]
- Update .gitignore [a0a7e66]
- Split WUPS 0.1 and Aroma port out [0c10bcc]
- Add .editorconfig [34ee1bd]
- Rebrand to Inkay [3448ef7]
- main: change URL to Pretendo [521b02d]
- Add WUPS v0.1 Dockerfile [aab0a16]
- Initial commit (works, but bad) [ec64a7c]

