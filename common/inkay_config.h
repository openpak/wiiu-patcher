//
// Created by ash on 17/12/24.
//

#ifndef INKAY_INKAY_CONFIG_H
#define INKAY_INKAY_CONFIG_H

#ifdef __has_include
#if __has_include("inkay_config.local.h")

#include "inkay_config.local.h"
#define INKAY_CUSTOM 1

#endif
#endif

#ifndef NETWORK_BASEURL
#define NETWORK_BASEURL "openpak.org"
#endif

// OpenPak server addresses: the single source, fixed at build time (Makefile variables
// OPENPAK_SERVER_IP, OPENPAK_NNCS2_IP, OPENPAK_IOSU_PORT). Nothing here needs a DNS record:
// - PPC side (games, NEX, applets): dns_hooks.cpp answers every *.openpak.org name itself.
// - IOSU side (act, nim, boss): iosu_url_patches.h writes IP-literal URLs, because IOSU
//   resolves through the console's real DNS and never sees the PPC hook.
// A server move therefore needs a new Inkay release.
#ifndef OPENPAK_SERVER_IP
#define OPENPAK_SERVER_IP "145.241.199.19"
#endif
#ifndef OPENPAK_NNCS2_IP
#define OPENPAK_NNCS2_IP "145.241.228.207"
#endif
// TLS port of the Wii U system entrypoint on the box (openpak-edge terminates TLS with the
// console certificate as its default, Traefik routes by path). Keep it four digits: the act
// URL slot is exactly as long as "account.openpak.org", which "<ip>:<4-digit port>" matches
// for the default address.
#ifndef OPENPAK_IOSU_PORT
#define OPENPAK_IOSU_PORT "4443"
#endif
#define OPENPAK_IOSU_HTTPS "https://" OPENPAK_SERVER_IP ":" OPENPAK_IOSU_PORT
#define OPENPAK_IOSU_HTTP "http://" OPENPAK_SERVER_IP

#endif //INKAY_INKAY_CONFIG_H
