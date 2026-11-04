#include "redirect_fetch.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#include <curl/curl.h>
#include <nsysnet/_socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>

#include "module.h"
#include "utils/logger.h"

namespace {
// The public origin, over public TLS trust (the CURLWrapperModule carries the CA bundle). The
// name openpak.org itself is never redirected by the module, so this never goes through the
// OpenPak CA or a DNS override.
const char *const URL = "https://openpak.org/api/v1/network/ceiling";
const char *const DIR = "fs:/vol/external01/wiiu/openpak";
const char *const CACHE = "fs:/vol/external01/wiiu/openpak/redirects.json";
const size_t MAX_BYTES = 256 * 1024;

std::thread worker;
std::atomic<bool> stop{false};

size_t sink(char *p, size_t sz, size_t n, void *ud) {
    auto *s = static_cast<std::string *>(ud);
    if (s->size() + sz * n > MAX_BYTES) return 0;
    s->append(p, sz * n);
    return sz * n;
}

bool readFile(const char *path, std::string *out) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    char b[1024];
    size_t n;
    while ((n = fread(b, 1, sizeof(b), f)) > 0 && out->size() < MAX_BYTES) out->append(b, n);
    fclose(f);
    return !out->empty();
}

// Written as received (not re-serialised), through a temp file and a rename.
void writeCache(const std::string &bytes) {
    mkdir("fs:/vol/external01/wiiu", 0777);
    mkdir(DIR, 0777);
    std::string tmp = std::string(CACHE) + ".tmp";
    FILE *f = fopen(tmp.c_str(), "wb");
    if (!f) return;
    bool ok = fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size();
    ok = fclose(f) == 0 && ok;
    if (ok) {
        unlink(CACHE);
        rename(tmp.c_str(), CACHE);
    } else {
        unlink(tmp.c_str());
    }
}

bool fetch(std::string *body) {
    CURL *c = curl_easy_init();
    if (!c) return false;
    body->clear();
    curl_easy_setopt(c, CURLOPT_URL, URL);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, sink);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, body);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L);
    CURLcode rc = curl_easy_perform(c);
    long code = 0;
    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(c);
    return rc == CURLE_OK && code == 200 && !body->empty();
}

void run() {
    // The console boots Aroma before its network is up: try for about two minutes, then leave
    // the cached or bundled list in place.
    socket_lib_init();
    for (int attempt = 0; attempt < 24 && !stop; attempt++) {
        std::string body;
        if (fetch(&body)) {
            int32_t v = Inkay_SetRedirectList(body.data(), (uint32_t) body.size());
            if (v > 0) {
                DEBUG_FUNCTION_LINE("Inkay/redirects: signed list v%d accepted", v);
                writeCache(body);
            } else {
                DEBUG_FUNCTION_LINE("Inkay/redirects: fetched list refused (%d)", v);
            }
            return;   // a refused list is not retried: that would only hammer the server
        }
        for (int i = 0; i < 50 && !stop; i++) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
}  // namespace

void RedirectList_Start() {
    std::string cached;
    if (readFile(CACHE, &cached)) {
        int32_t v = Inkay_SetRedirectList(cached.data(), (uint32_t) cached.size());
        DEBUG_FUNCTION_LINE("Inkay/redirects: cache %s (%d)", v > 0 ? "loaded" : "refused", v);
    }
    stop = false;
    worker = std::thread(run);
}

void RedirectList_Stop() {
    stop = true;
    if (worker.joinable()) worker.join();
}
