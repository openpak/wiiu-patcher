#ifndef _PATCHER_H
#define _PATCHER_H

#include "inkay_config.h"
#include <string_view>

// IOSU-side URLs. IOSU (act, nim, boss) resolves hostnames through the console's own DNS
// settings, not through the PPC hook in dns_hooks.cpp, so these are IP literals: the console
// reaches OpenPak on automatic DNS with no public record. https URLs go to the Wii U system
// entrypoint (OPENPAK_IOSU_PORT), which serves the console certificate without SNI and routes
// by path; the http ones go to port 80 like before.
//
// `slot` is the length of the string the slot held before (the openpak.org name, which has
// run on retail consoles): a replacement must not be longer, or it overwrites what follows
// it in IOSU memory. The static_assert below enforces that for any build-time address.

typedef struct URL_Patch {
    unsigned int address;
    unsigned int slot; // sizeof the previous string, terminator included
    char url[80];
} URL_Patch;

#define IOSU_URL(addr, previous, replacement) {addr, sizeof(previous), replacement}

static constexpr URL_Patch url_patches[] = {
        // Deliberately absent: tagaya (0xE2281964, 0xE22819B4) and nus.wup (0xE22830E0). Game and
        // system update traffic stays on the stock Nintendo URLs, so the console fetches the
        // version list from tagaya[-wup].nintendo.net and the title TMD/contents from Nintendo's
        // CDN itself. Also absent: idbe (0xE22AB2D8, 0xE22AB318, 0xE22AB358, 0xE22AB398), so title
        // icons come from idbe-wup/idbe-ctr.cdn.nintendo.net; a 404 there is error 105-1185 (card #127).
        // ecs.wup stays here: our ECS answers with ccs.cdn.wup.shop.nintendo.net URLs.
        //nim-boss .rodata
        IOSU_URL(0xE2282550, "http://pushmore.wup.shop.openpak.org/pushmore/r/%s",
                 OPENPAK_IOSU_HTTP "/pushmore/r/%s"),
        IOSU_URL(0xE229A0A0, "http://npns-dev.c.app.openpak.org/bst.dat",
                 OPENPAK_IOSU_HTTP "/bst.dat"),
        IOSU_URL(0xE229A0D0, "http://npns-dev.c.app.openpak.org/bst2.dat",
                 OPENPAK_IOSU_HTTP "/bst2.dat"),
        IOSU_URL(0xE2282584, "http://pushmo.wup.shop.openpak.org/pushmo/d/%s/%u",
                 OPENPAK_IOSU_HTTP "/pushmo/d/%s/%u"),
        IOSU_URL(0xE22825B8, "http://pushmo.wup.shop.openpak.org/pushmo/c/%u/%u",
                 OPENPAK_IOSU_HTTP "/pushmo/c/%u/%u"),
        IOSU_URL(0xE2282DB4, "https://ecs.wup.shop.openpak.org/ecs/services/ECommerceSOAP",
                 OPENPAK_IOSU_HTTPS "/ecs/services/ECommerceSOAP"),
        IOSU_URL(0xE22830A0, "https://ecs.wup.shop.openpak.org/ecs/services/ECommerceSOAP",
                 OPENPAK_IOSU_HTTPS "/ecs/services/ECommerceSOAP"),
        // Bare host of the policy list (the .bss URL below is built from it at IOSU start).
        IOSU_URL(0xE2299990, "nppl.app.openpak.org",
                 OPENPAK_SERVER_IP ":" OPENPAK_IOSU_PORT),
        IOSU_URL(0xE229A600, "https://pls.wup.shop.openpak.org/pls/upload",
                 OPENPAK_IOSU_HTTPS "/pls/upload"),
        IOSU_URL(0xE229A6AC, "https://npvk-dev.app.openpak.org/reports",
                 OPENPAK_IOSU_HTTPS "/reports"),
        IOSU_URL(0xE229A6D8, "https://npvk.app.openpak.org/reports",
                 OPENPAK_IOSU_HTTPS "/reports"),
        IOSU_URL(0xE229B1F4, "https://npts.app.openpak.org/p01/tasksheet/%s/%s/%s/%s?c=%s&l=%s",
                 OPENPAK_IOSU_HTTPS "/p01/tasksheet/%s/%s/%s/%s?c=%s&l=%s"),
        IOSU_URL(0xE229B238, "https://npts.app.openpak.org/p01/tasksheet/%s/%s/%s?c=%s&l=%s",
                 OPENPAK_IOSU_HTTPS "/p01/tasksheet/%s/%s/%s?c=%s&l=%s"),
        IOSU_URL(0xE22B3EF8, "https://ecs.c.shop.openpak.org",
                 OPENPAK_IOSU_HTTPS),
        IOSU_URL(0xE22B3F30, "https://ecs.c.shop.openpak.org/ecs/services/ECommerceSOAP",
                 OPENPAK_IOSU_HTTPS "/ecs/services/ECommerceSOAP"),
        IOSU_URL(0xE22B3F70, "https://ias.c.shop.openpak.org/ias/services/IdentityAuthenticationSOAP",
                 OPENPAK_IOSU_HTTPS "/ias/services/IdentityAuthenticationSOAP"),
        IOSU_URL(0xE22B3FBC, "https://cas.c.shop.openpak.org/cas/services/CatalogingSOAP",
                 OPENPAK_IOSU_HTTPS "/cas/services/CatalogingSOAP"),
        IOSU_URL(0xE22B3FFC, "https://nus.c.shop.openpak.org/nus/services/NetUpdateSOAP",
                 OPENPAK_IOSU_HTTPS "/nus/services/NetUpdateSOAP"),
        // Unmeasured: a bare name, possibly a suffix that IOSU prepends to, so it cannot become
        // an IP. It stays a name (and needs DNS) until a console capture shows how it is used.
        IOSU_URL(0xE229DE0C, "n.app.openpak.org",
                 "n.app." NETWORK_BASEURL),
        //nim-boss .bss
        IOSU_URL(0xE24B8A24, "https://nppl.app.openpak.org/p01/policylist/1/1/UNK",
                 OPENPAK_IOSU_HTTPS "/p01/policylist/1/1/UNK"),
        // IOS-ACT. The two %s are environment prefixes, empty on retail consoles.
        IOSU_URL(0xE31930D4, "https://%s%saccount.openpak.org/v%u/api/",
                 "https://%s%s" OPENPAK_SERVER_IP ":" OPENPAK_IOSU_PORT "/v%u/api/"),
};

#undef IOSU_URL

static constexpr bool iosu_urls_fit() {
    for (const auto &patch: url_patches) {
        if (std::string_view(patch.url).size() + 1 > patch.slot) return false;
    }
    return true;
}
static_assert(iosu_urls_fit(), "an IOSU URL is longer than its slot: shorten OPENPAK_SERVER_IP/OPENPAK_IOSU_PORT");

#endif
