#include "Backup.hpp"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <vector>

namespace {
std::string g_root;
const Backup::Item *g_items = nullptr;
size_t g_count = 0;
const char *CHANGED = "System files changed; originals kept in /wiiu/openpak/system";

std::string state(const Backup::Item &it, const char *ext) { return g_root + "/wiiu/openpak/system/" + it.key + "." + ext; }
std::string target(const Backup::Item &it) { return g_root + "/" + it.target; }

bool exists(const std::string &p) { struct stat st; return stat(p.c_str(), &st) == 0; }

bool read(const std::string &p, std::vector<unsigned char> &out) {
    FILE *f = fopen(p.c_str(), "rb");
    if (!f) return false;
    out.clear();
    unsigned char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out.insert(out.end(), buf, buf + n);
    bool ok = !ferror(f);
    fclose(f);
    return ok;
}

bool same(const std::string &a, const std::string &b) {
    std::vector<unsigned char> x, y;
    return read(a, x) && read(b, y) && x == y;
}

bool sameBytes(const std::string &a, const unsigned char *d, size_t n) {
    std::vector<unsigned char> x;
    return read(a, x) && x.size() == n && (n == 0 || memcmp(x.data(), d, n) == 0);
}

void mkdirs(const std::string &file) {
    std::string p = file;
    for (size_t i = 1; i < p.size(); ++i)
        if (p[i] == '/') { p[i] = 0; mkdir(p.c_str(), 0777); p[i] = '/'; }
}

bool writeBytes(const std::string &dst, const unsigned char *d, size_t n) {
    mkdirs(dst);
    std::string tmp = dst + ".openpak-tmp";
    FILE *f = fopen(tmp.c_str(), "wb");
    if (!f) return false;
    bool ok = n == 0 || fwrite(d, 1, n, f) == n;
    ok = fclose(f) == 0 && ok;
    if (!ok) { std::remove(tmp.c_str()); return false; }
    std::remove(dst.c_str());
    return std::rename(tmp.c_str(), dst.c_str()) == 0;
}

bool copy(const std::string &src, const std::string &dst) {
    std::vector<unsigned char> data;
    return read(src, data) && writeBytes(dst, data.data(), data.size());
}

bool mark(const std::string &p) { return writeBytes(p, nullptr, 0); }

bool ours(const Backup::Item &it) {
    std::string t = target(it);
    if (!exists(t)) return false;
    return (exists(state(it, "managed")) && same(t, state(it, "managed"))) ||
           (exists(state(it, "previous")) && same(t, state(it, "previous")));
}
}

namespace Backup {

void init(const std::string &root, const Item *items, size_t count) { g_root = root; g_items = items; g_count = count; }

int stage(char *err, size_t errlen) {
    int changed = 0;
    for (size_t i = 0; i < g_count; ++i) {
        const Item &it = g_items[i];
        std::string m = state(it, "managed");
        if (exists(m) && sameBytes(m, it.data, it.size)) continue;
        // The copy an earlier release installed: still recognised as OpenPak's until it is replaced.
        if (exists(m) && !exists(state(it, "previous")) && !copy(m, state(it, "previous"))) {
            snprintf(err, errlen, "Could not stage %s", it.key); return -1;
        }
        if (!writeBytes(m, it.data, it.size)) { snprintf(err, errlen, "Could not stage %s", it.key); return -1; }
        ++changed;
    }
    return changed;
}

bool wasInstalled() {
    for (size_t i = 0; i < g_count; ++i) if (ours(g_items[i])) return true;
    return false;
}

int liveCount() {
    int n = 0;
    for (size_t i = 0; i < g_count; ++i) {
        std::string t = target(g_items[i]);
        if (exists(t) && sameBytes(t, g_items[i].data, g_items[i].size)) ++n;
    }
    return n;
}

bool installed() { return g_count > 0 && liveCount() == (int)g_count; }

bool install(char *err, size_t errlen) {
    // Check everything first, so a file somebody else changed stops the switch before any write.
    for (size_t i = 0; i < g_count; ++i) {
        const Item &it = g_items[i];
        std::string t = target(it);
        bool recorded = exists(state(it, "original")) || exists(state(it, "absent"));
        if (recorded && exists(t) && !ours(it) && !(exists(state(it, "original")) && same(t, state(it, "original")))) {
            snprintf(err, errlen, "%s", CHANGED);
            return false;
        }
    }
    for (size_t i = 0; i < g_count; ++i) {
        const Item &it = g_items[i];
        std::string t = target(it);
        if (!exists(state(it, "original")) && !exists(state(it, "absent"))) {
            bool ok;
            if (it.own && exists(t) && !ours(it)) ok = copy(t, state(it, "replaced")) && mark(state(it, "absent"));
            else ok = (exists(t) && !ours(it)) ? copy(t, state(it, "original")) : mark(state(it, "absent"));
            if (!ok) { snprintf(err, errlen, "Could not back up %s", it.key); return false; }
        }
        if (!(exists(t) && sameBytes(t, it.data, it.size)) && !writeBytes(t, it.data, it.size)) {
            snprintf(err, errlen, "Could not install %s", it.key);
            return false;
        }
        std::remove(state(it, "previous").c_str());
    }
    return true;
}

bool remove(char *err, size_t errlen) {
    for (size_t i = 0; i < g_count; ++i) {
        const Item &it = g_items[i];
        std::string t = target(it);
        if (!exists(state(it, "original")) && !exists(state(it, "absent"))) continue;
        if (!exists(t)) continue;
        if (!ours(it) && !(exists(state(it, "original")) && same(t, state(it, "original")))) {
            snprintf(err, errlen, "%s", CHANGED);
            return false;
        }
    }
    for (size_t i = 0; i < g_count; ++i) {
        const Item &it = g_items[i];
        std::string t = target(it);
        bool hasOrig = exists(state(it, "original")), hasAbsent = exists(state(it, "absent"));
        if (!hasOrig && !hasAbsent) continue;
        if (hasOrig) {
            if (!(exists(t) && same(t, state(it, "original"))) && !copy(state(it, "original"), t)) {
                snprintf(err, errlen, "Could not restore system setup; originals kept");
                return false;
            }
        } else if (exists(t)) {
            std::remove(t.c_str());
        }
        std::remove(state(it, "original").c_str());
        std::remove(state(it, "absent").c_str());
        std::remove(state(it, "previous").c_str());
    }
    return true;
}

}
