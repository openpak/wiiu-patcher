#include "redirect_list.h"

#include <algorithm>
#include <cstring>
#include <map>

#include "ed25519.h"

namespace {
// Pinned public key(s). A list so a new key can be added before the old one is retired.
struct PinnedKey { const char *keyid; uint8_t pub[32]; };
const PinnedKey KEYS[] = {
    {"36e8bcdd93c2c1a1d7e5d87bbba05a7a4f97882131cf6878f1c053a25a377fe4",
     {0x0f, 0xd2, 0xa2, 0x66, 0x08, 0x68, 0xe5, 0x3d, 0x20, 0xfc, 0x81, 0x1e, 0x3f, 0x15, 0x71, 0x0c,
      0xb1, 0xad, 0x15, 0x7b, 0xa7, 0xda, 0x80, 0x19, 0xf7, 0xa5, 0x9e, 0x8a, 0xd6, 0x04, 0xd8, 0x7c}},
};

// Always stock, whatever the signed list says: game and system updates stay with Nintendo
// (board #127), and the connection test measures the player's own internet.
const char *const COMPILED_NEVER[] = {
    // Update path: the version list (tagaya, tagaya-wup.cdn), NetUpdate (nus), title metadata and
    // contents (ccs, ccs.cdn, nus.cdn) and every other shop/CDN host Nintendo serves updates from.
    // None of these is ever redirected on a Wii U, whatever family the signed list holds.
    // Title icons (idbe-wup.cdn.nintendo.net, idbe-ctr.cdn.nintendo.net) are covered by .cdn.nintendo.net.
    ".shop.nintendo.net", ".shop.nintendowifi.net", ".cdn.nintendo.net", ".cdn.nintendowifi.net",
    "tagaya.wup.shop.nintendo.net", "nus.wup.shop.nintendo.net",
    // The connection test measures the player's own internet.
    "conntest.nintendowifi.net",
    // Where the list itself is fetched, over public TLS.
    "openpak.org"};

bool b64decode(const std::string &s, std::vector<uint8_t> *out) {
    int val = 0, bits = -8;
    for (unsigned char c : s) {
        int d;
        if (c >= 'A' && c <= 'Z') d = c - 'A';
        else if (c >= 'a' && c <= 'z') d = c - 'a' + 26;
        else if (c >= '0' && c <= '9') d = c - '0' + 52;
        else if (c == '+') d = 62;
        else if (c == '/') d = 63;
        else if (c == '=') break;
        else return false;
        val = (val << 6) | d;
        bits += 6;
        if (bits >= 0) { out->push_back((uint8_t)((val >> bits) & 0xff)); bits -= 8; }
    }
    return true;
}

// ---- a small JSON reader: enough for the envelope and the payload -------------------------
struct J {
    enum T { NUL, BOOL, NUM, STR, ARR, OBJ } t = NUL;
    std::string s;
    double n = 0;
    std::vector<J> a;
    std::vector<std::pair<std::string, J>> o;
    const J *get(const char *k) const {
        for (auto &kv : o) if (kv.first == k) return &kv.second;
        return nullptr;
    }
};

struct P {
    const char *p, *e;
    int depth = 0;
    void ws() { while (p < e && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++; }
    bool str(std::string *out) {
        if (p >= e || *p != '"') return false;
        p++;
        while (p < e && *p != '"') {
            if (*p == '\\') {
                p++;
                if (p >= e) return false;
                switch (*p) {
                    case 'n': out->push_back('\n'); break;
                    case 't': out->push_back('\t'); break;
                    case 'r': out->push_back('\r'); break;
                    case 'b': case 'f': break;
                    case 'u': if (e - p < 5) return false; out->push_back('?'); p += 4; break;
                    default: out->push_back(*p);
                }
                p++;
            } else {
                out->push_back(*p++);
            }
        }
        if (p >= e) return false;
        p++;
        return true;
    }
    bool val(J *j) {
        if (++depth > 16) return false;
        ws();
        if (p >= e) return false;
        bool ok = false;
        if (*p == '"') { j->t = J::STR; ok = str(&j->s); }
        else if (*p == '{') {
            j->t = J::OBJ; p++; ws();
            if (p < e && *p == '}') { p++; ok = true; }
            else for (;;) {
                std::string k; J v;
                ws();
                if (!str(&k)) break;
                ws();
                if (p >= e || *p != ':') break;
                p++;
                if (!val(&v)) break;
                j->o.emplace_back(std::move(k), std::move(v));
                ws();
                if (p < e && *p == ',') { p++; continue; }
                if (p < e && *p == '}') { p++; ok = true; }
                break;
            }
        } else if (*p == '[') {
            j->t = J::ARR; p++; ws();
            if (p < e && *p == ']') { p++; ok = true; }
            else for (;;) {
                J v;
                if (!val(&v)) break;
                j->a.push_back(std::move(v));
                ws();
                if (p < e && *p == ',') { p++; continue; }
                if (p < e && *p == ']') { p++; ok = true; }
                break;
            }
        } else if (!strncmp(p, "true", 4) && e - p >= 4) { j->t = J::BOOL; p += 4; ok = true; }
        else if (!strncmp(p, "false", 5) && e - p >= 5) { j->t = J::BOOL; p += 5; ok = true; }
        else if (!strncmp(p, "null", 4) && e - p >= 4) { p += 4; ok = true; }
        else {
            const char *q = p;
            while (q < e && (strchr("+-.eE", *q) || (*q >= '0' && *q <= '9'))) q++;
            if (q > p) { j->t = J::NUM; j->n = strtod(std::string(p, q).c_str(), nullptr); p = q; ok = true; }
        }
        depth--;
        return ok;
    }
};

bool parse_json(const char *b, size_t n, J *j) {
    P p{b, b + n};
    if (!p.val(j)) return false;
    p.ws();
    return p.p == p.e;
}

// A family: ".", lower case, letters/digits/-/., at least two labels.
bool valid_family(const std::string &f) {
    if (f.size() < 4 || f[0] != '.') return false;
    int labels = 0;
    for (size_t i = 0; i < f.size(); i++) {
        char c = f[i];
        if (c == '.') { if (i + 1 >= f.size() || f[i + 1] == '.') return false; labels++; }
        else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return false;
    }
    return labels >= 2;
}

bool add_families(const J &arr, std::vector<std::string> *dst) {
    if (arr.t != J::ARR) return false;
    for (auto &f : arr.a) {
        if (f.t != J::STR || !valid_family(f.s)) return false;
        dst->push_back(f.s);
    }
    return true;
}

std::string norm(const char *host) {
    std::string h = host ? host : "";
    while (!h.empty() && h.back() == '.') h.pop_back();
    for (auto &c : h) if (c >= 'A' && c <= 'Z') c += 32;
    return h;
}

bool covers(const std::string &entry, const std::string &h) {
    if (entry.empty()) return false;
    if (entry[0] != '.') return h == entry;
    if (h.size() + 1 == entry.size()) return h == entry.substr(1);
    return h.size() >= entry.size() && h.compare(h.size() - entry.size(), entry.size(), entry) == 0;
}
}  // namespace

RedirectList redirect_bundled() {
    RedirectList l;
    // What dns_hooks.cpp redirected before the signed list: OpenPak's own names and the two NAT check hosts.
    l.families = {".openpak.org", "nncs1.app.nintendowifi.net", "nncs2.app.nintendowifi.net"};
    for (auto n : COMPILED_NEVER) l.never.push_back(n);
    return l;
}

bool redirect_parse_envelope(const char *bytes, size_t len, RedirectList *out, std::string *err) {
    auto fail = [&](const char *m) { if (err) *err = m; return false; };
    J env;
    if (!parse_json(bytes, len, &env) || env.t != J::OBJ) return fail("not JSON");
    const J *p64 = env.get("payload"), *sigs = env.get("signatures");
    if (!p64 || p64->t != J::STR || !sigs || sigs->t != J::ARR) return fail("not an envelope");
    std::vector<uint8_t> payload;
    if (!b64decode(p64->s, &payload) || payload.empty()) return fail("payload is not base64");
    bool ok = false;
    for (auto &s : sigs->a) {
        const J *id = s.get("keyid"), *sg = s.get("sig");
        if (s.t != J::OBJ || !id || !sg || id->t != J::STR || sg->t != J::STR) continue;
        std::vector<uint8_t> raw;
        if (!b64decode(sg->s, &raw) || raw.size() != 64) continue;
        for (auto &k : KEYS)
            if (id->s == k.keyid && ed25519_verify(raw.data(), payload.data(), payload.size(), k.pub)) ok = true;
    }
    if (!ok) return fail("no valid signature under a pinned key");

    J root;
    if (!parse_json((const char *) payload.data(), payload.size(), &root) || root.t != J::OBJ) return fail("payload is not JSON");
    const J *type = root.get("type"), *ver = root.get("version");
    if (!type || type->t != J::STR || type->s != "openpak-ceiling") return fail("wrong payload type");
    if (!ver || ver->t != J::NUM || ver->n < 1) return fail("bad version");

    std::vector<std::string> fam, never;
    if (const J *all = root.get("all")) {
        if (!add_families(*all, &fam)) return fail("malformed family in all");
    } else if (const J *plats = root.get("platforms"); plats && plats->t == J::OBJ) {
        for (auto &kv : plats->o)
            if (!add_families(kv.second, &fam)) return fail("malformed family in platforms");
    } else {
        return fail("no families");
    }
    if (const J *nv = root.get("never")) {
        if (nv->t != J::ARR) return fail("malformed never");
        for (auto &x : nv->a) if (x.t == J::STR && !x.s.empty()) never.push_back(x.s);
    }

    RedirectList l = redirect_bundled();
    size_t before = l.families.size();
    l.version = (int64_t) ver->n;
    for (auto &f : fam)
        if (std::find(l.families.begin(), l.families.end(), f) == l.families.end()) l.families.push_back(f);
    for (auto &n : never) l.never.push_back(n);
    l.signedCount = l.families.size() - before;
    *out = std::move(l);
    return true;
}

bool redirect_matches(const RedirectList &list, const char *host) {
    std::string h = norm(host);
    if (h.empty()) return false;
    for (auto &n : list.never) if (covers(n, h)) return false;
    for (auto &f : list.families) if (covers(f, h)) return true;
    return false;
}
