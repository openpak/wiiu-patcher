// Host test of the signed redirect list: redirect_test <signed-envelope.json>
// The envelope is a real ceiling file; the test checks the signature path, tampering and matching.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include "redirect_list.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(int argc, char **argv) {
    if (argc < 2) return 2;
    std::ifstream f(argv[1], std::ios::binary);
    std::stringstream ss; ss << f.rdbuf();
    std::string env = ss.str(), err;

    RedirectList base = redirect_bundled();
    CHECK(redirect_matches(base, "account.openpak.org"));
    CHECK(!redirect_matches(base, "openpak.org"));
    CHECK(!redirect_matches(base, "tagaya.wup.shop.nintendo.net"));
    CHECK(!redirect_matches(base, "example.com"));

    RedirectList l;
    bool pe = redirect_parse_envelope(env.data(), env.size(), &l, &err);
    if (!pe) printf("err: %s\n", err.c_str());
    CHECK(pe);
    printf("version %lld, %zu entries (%zu signed)\n", (long long) l.version, l.count(), l.signedCount);
    CHECK(l.version > 0);
    CHECK(redirect_matches(l, "Backend.Klei.com"));          // another console's family still matches
    CHECK(redirect_matches(l, "klei.com"));
    CHECK(redirect_matches(l, "x.demonware.net."));
    CHECK(!redirect_matches(l, "notklei.com"));
    CHECK(!redirect_matches(l, "tagaya.wup.shop.nintendo.net"));
    CHECK(!redirect_matches(l, "nus.wup.shop.nintendo.net"));
    CHECK(!redirect_matches(l, "conntest.nintendowifi.net"));
    CHECK(!redirect_matches(l, "openpak.org"));              // the list itself is fetched unredirected
    CHECK(!redirect_matches(l, "google.com"));

    // Update hosts (#127 map plus the content CDNs): never redirected, in the bundled list and in
    // the live ceiling alike, and the live union does contain their parent families.
    CHECK(redirect_matches(l, "something.nintendo.net"));
    CHECK(redirect_matches(l, "something.nintendowifi.net"));
    const char *updates[] = {"tagaya.wup.shop.nintendo.net", "tagaya-wup.cdn.nintendo.net", "tagaya.cdn.nintendo.net",
        "nus.wup.shop.nintendo.net", "nus.cdn.wup.shop.nintendo.net", "ccs.wup.shop.nintendo.net",
        "ccs.cdn.wup.shop.nintendo.net", "ccs.cdn.c.shop.nintendo.net", "nus.c.shop.nintendo.net",
        "ecs.wup.shop.nintendo.net", "ias.wup.shop.nintendo.net", "ccs.c.shop.nintendowifi.net",
        "nus.c.shop.nintendowifi.net", "ecs.c.shop.nintendowifi.net", "ccs.cdn.c.shop.nintendowifi.net",
        "conntest.nintendowifi.net"};
    for (const char *h : updates) {
        if (redirect_matches(l, h) || redirect_matches(base, h)) { printf("FAIL update host redirected: %s\n", h); fails++; }
    }
    // The eShop SOAP hosts Inkay rewrites by URL are OpenPak names (and IOSU takes IP literals, so
    // it never asks the PPC resolver); they stay on OpenPak.
    const char *openpak[] = {"ecs.wup.shop.openpak.org", "ias.wup.shop.openpak.org", "cas.c.shop.openpak.org",
        "ecs.c.shop.openpak.org", "ias.c.shop.openpak.org", "nus.c.shop.openpak.org", "account.openpak.org"};
    for (const char *h : openpak) {
        if (!redirect_matches(l, h) || !redirect_matches(base, h)) { printf("FAIL OpenPak host not redirected: %s\n", h); fails++; }
    }

    // Tampering: change one character of the signed payload and the signature must fail.
    std::string bad = env;
    size_t k = bad.find("\"payload\":\"") + 14;
    bad[k] = bad[k] == 'A' ? 'B' : 'A';
    RedirectList t = base;
    CHECK(!redirect_parse_envelope(bad.data(), bad.size(), &t, &err));
    CHECK(t.version == 0);
    CHECK(!redirect_parse_envelope("{}", 2, &t, &err));
    CHECK(!redirect_parse_envelope("", 0, &t, &err));
    printf(fails ? "FAILED\n" : "ok\n");
    return fails ? 1 : 0;
}
