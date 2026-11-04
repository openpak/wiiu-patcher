// The network-wide signed redirect list (docs/signed-ceiling.md in openpak-client).
//
// One signed envelope for the whole network: the console redirects a game-server name when it
// ends in one of the list's families, whatever console the family was added for. Names that
// never occur on a Wii U simply never match. The envelope is the ceiling file the Switch and
// the emulators already verify (Ed25519, pinned key, version that only grows), so one signing
// serves every platform. The list is `all` when the payload carries it, otherwise the union of
// every platform's families.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct RedirectList {
    int64_t version = 0;                 // 0 = the bundled list only
    std::vector<std::string> families;   // ".example.com": the apex and everything below it
    std::vector<std::string> never;      // exact names or ".suffix" that always stay stock
    size_t signedCount = 0;              // how many entries came from the signed list
    size_t count() const { return families.size(); }
};

// Verifies an envelope (signature under a pinned key, payload shape) and returns the list:
// the bundled base entries plus the signed ones. On failure returns false with `err` set and
// leaves `out` untouched.
bool redirect_parse_envelope(const char *bytes, size_t len, RedirectList *out, std::string *err);

// The bundled base list (what Inkay redirected before the signed list existed).
RedirectList redirect_bundled();

// True when `host` (any case, optional trailing dot) is redirected.
bool redirect_matches(const RedirectList &list, const char *host);
