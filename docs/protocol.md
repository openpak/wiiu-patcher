# Protocol: wiiu-patcher

Fork of Pretendo's Inkay (GPL/AGPL as upstream). The same in-memory patches, pointed at
OpenPak: every `*.pretendo.cc` name became the same-length `*.openpak.org` name, and
`data/ca.pem` is the OpenPak Network CA. Copy `Inkay-openpak.wms` and `Inkay-openpak.wps` to the
Aroma folders and the console goes to OpenPak; remove them and it goes back. Releases: `vX.Y.Z`
tags.

## No DNS record needed

A console with Inkay-openpak reaches OpenPak on automatic DNS, through any home resolver, with
nothing published for us in public DNS. There are two halves, because the Wii U has two network
stacks:

| Side | Who | How it reaches OpenPak |
| --- | --- | --- |
| PPC (Cafe OS) | games, NEX, Miiverse, eShop and account applets | `src/patches/dns_hooks.cpp` hooks `gethostbyname`/`getaddrinfo`: every `*.openpak.org` name (and `nncs1`/`nncs2.app.nintendowifi.net`) answers with `OPENPAK_SERVER_IP` (nncs2 with `OPENPAK_NNCS2_IP`) without asking a resolver. Hostnames stay in the URLs, so SNI and Host still pick the service. |
| IOSU (ARM) | IOS-ACT (account), IOS-NIM/BOSS (SOAP, update list, SpotPass, policy list, icons) | IOSU resolves through the console's own DNS settings and never sees the PPC hook, so `src/iosu_url_patches.h` writes IP-literal URLs: `https://<OPENPAK_SERVER_IP>:<OPENPAK_IOSU_PORT>/...`, and `http://<OPENPAK_SERVER_IP>/...` for the plain-HTTP ones. |

The box side of the IOSU half: public TCP `OPENPAK_IOSU_PORT` (4443) is the Wii U system
entrypoint. `openpak-edge` terminates TLS there with the console certificate as its default
(an IP URL sends no SNI; the certificate carries `IP:` and `DNS:` SANs for the address) and
relays plain HTTP, with the visitor address in PROXY v2, to Traefik's `wiiu-system` entrypoint.
That entrypoint routes by path and rewrites `Host` to the name each service expects:

| Path | Host given to the service | Service |
| --- | --- | --- |
| `/v1/api/` | `account.openpak.org` | nn-account |
| `/ecs/`, `/ias/`, `/pls/` | `<x>.wup.shop.openpak.org` | nn-soap |
| `/cas/` | `cas.c.shop.openpak.org` | nn-soap |
| `/p01/tasksheet/` | `npts.app.openpak.org`, plus `X-OpenPak-Npdi-Base` so tasksheet data URLs are IP literals too | nn-boss |
| `/p01/policylist/`, `/p01/data/` | `nppl.app.openpak.org`, `npdi.cdn.openpak.org` | nn-boss |

`/icondata`, `/reports` and the plain-HTTP `/pushmo`, `/pushmore`, `/bst.dat` have no OpenPak
service yet (404, as with the names).

Why IP literals and not a hook in the IOSU resolver: the URL patches already exist at known
addresses and are plain string writes through Mocha; hooking IOS-NET's resolver means ARM code
injection at per-firmware addresses with no dump to measure them against. The IP literal costs
the TLS name check (NoSSL is on today; the IP SAN keeps a CA-only future working) and Host
routing (done by path on the box).

### Trade-off: a server move is a release

The addresses are compiled in (`common/inkay_config.h`, Makefile variables
`OPENPAK_SERVER_IP`, `OPENPAK_NNCS2_IP`, `OPENPAK_IOSU_PORT`). Moving the box to another address
means a new Inkay-openpak release, and consoles on the old one keep calling the old address. A
build fails (static_assert) if an address makes any IOSU string longer than the slot it
replaces; with a four-digit port the account URL fits any IPv4 address up to 14 characters
exactly, and the 15-character ones fit Nintendo's original slot but not ours.

### Not yet measured

- `n.app.openpak.org` (nim-boss `0xE229DE0C`) stays a name: it may be a suffix IOSU prepends to,
  so it cannot become an IP until a capture shows how it is used.
- The IOS-ACT URL's two `%s` prefixes are empty on retail consoles; a dev-environment console
  would prepend them to the IP.
- The addresses are the 5.5.5/5.5.6 IOSU layout (as upstream).
