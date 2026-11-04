/*  Copyright 2024 Pretendo Network contributors <pretendo.network>
    Copyright 2024 Ash Logan <ash@heyquark.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <netdb.h>

#include "config.h"
#include "utils/logger.h"
#include "inkay_config.h"
#include <array>
#include <vector>
#include <function_patcher/function_patching.h>

std::vector<PatchedFunctionHandle> dns_patches;

#include <cstring>
#include <atomic>
#include <string>
#include "redirect_list.h"

// OpenPak: the console is its own DNS for the names OpenPak serves, and only those. The
// redirect list is the bundled base (*.openpak.org and the two NAT check hosts) plus the
// network-wide signed list (common/redirect_list.h) once it is verified. A name on the list
// answers with the OpenPak box without asking a resolver (gethostbyname/getaddrinfo take a
// dotted-quad literal); every other name goes to the real resolver. Hostnames stay in the
// URLs, so SNI and Host still pick the service. The list's never-set keeps tagaya/nus (game
// updates), the connection test and the bare openpak.org (where the list itself is fetched
// over public TLS) on the real internet.
static std::atomic<RedirectList *> g_list{nullptr};

static const RedirectList *current_list() {
    RedirectList *l = g_list.load();
    if (!l) {
        RedirectList *fresh = new RedirectList(redirect_bundled());
        if (g_list.compare_exchange_strong(l, fresh)) l = fresh; else delete fresh;
    }
    return l;
}

static const char *replace_dns_name(const char *dns_name) {
    if (!Config::connect_to_network || !dns_name) return dns_name;
    if (!redirect_matches(*current_list(), dns_name)) return dns_name;
    if (strcmp(dns_name, "nncs1.app.nintendowifi.net") == 0) return OPENPAK_SERVER_IP;
    if (strcmp(dns_name, "nncs2.app.nintendowifi.net") == 0) return OPENPAK_NNCS2_IP;
    if (strcmp(dns_name, "nncs2.app." NETWORK_BASEURL) == 0) return OPENPAK_NNCS2_IP;
    return OPENPAK_SERVER_IP;
}

int32_t patchedRedirectSet(const char *envelope, uint32_t len) {
    RedirectList next;
    std::string err;
    if (!envelope || !redirect_parse_envelope(envelope, len, &next, &err)) {
        DEBUG_FUNCTION_LINE("Inkay/DNS: signed redirect list refused: %s", err.c_str());
        return -1;
    }
    const RedirectList *cur = current_list();
    if (next.version < cur->version) {   // rollback protection
        DEBUG_FUNCTION_LINE("Inkay/DNS: signed redirect list v%lld is older than v%lld", (long long) next.version, (long long) cur->version);
        return -2;
    }
    // The previous list is leaked on purpose: a hook on another thread may still be reading it.
    g_list.store(new RedirectList(std::move(next)));
    return (int32_t) current_list()->version;
}

void patchedRedirectInfo(uint32_t *names, int32_t *version) {
    const RedirectList *l = current_list();
    if (names) *names = (uint32_t) l->count();
    if (version) *version = (int32_t) l->version;
}

DECL_FUNCTION(struct hostent *, gethostbyname, const char *dns_name) {
    return real_gethostbyname(replace_dns_name(dns_name));
}

DECL_FUNCTION(int, getaddrinfo, const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    return real_getaddrinfo(replace_dns_name(node), service, hints, res);
}

void patchDNS() {
    dns_patches.reserve(2);

    auto add_patch = [](function_replacement_data_t repl, const char *name) {
        PatchedFunctionHandle handle = 0;
        if (FunctionPatcher_AddFunctionPatch(&repl, &handle, nullptr) != FUNCTION_PATCHER_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE("Inkay/DNS: Failed to patch %s!", name);
        }
        dns_patches.push_back(handle);
    };

    // might need a REPLACE_FUNCTION_FOR_PROCESS for Friends
    add_patch(REPLACE_FUNCTION(gethostbyname, LIBRARY_NSYSNET, gethostbyname), "gethostbyname");

    add_patch(REPLACE_FUNCTION(getaddrinfo, LIBRARY_NSYSNET, getaddrinfo), "getaddrinfo");
}

void unpatchDNS() {
    for (auto handle: dns_patches) {
        FunctionPatcher_RemoveFunctionPatch(handle);
    }
    dns_patches.clear();
}
