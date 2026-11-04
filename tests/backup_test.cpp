#include "Backup.hpp"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
static std::string R;
static void put(const std::string &p, const char *c) { std::string f = R + p; for (size_t i = 1; i < f.size(); ++i) if (f[i] == '/') mkdir(f.substr(0, i).c_str(), 0777); FILE *x = fopen(f.c_str(), "wb"); fputs(c, x); fclose(x); }
static std::string get(const std::string &p) { FILE *x = fopen((R + p).c_str(), "rb"); if (!x) return "<none>"; char b[256] = {0}; fread(b, 1, 255, x); fclose(x); return b; }
int main(int, char **argv) {
    R = argv[1]; char e[200] = "";
    unsigned char mod[] = "MOD1", plug[] = "PLUG1";
    Backup::Item items[] = {
        {"module", "wiiu/environments/aroma/modules/Inkay-openpak.wms", mod, 4, false},
        {"plugin", "wiiu/environments/aroma/plugins/Inkay-openpak.wps", plug, 5, false},
    };
    Backup::init(R, items, 2);
    put("/wiiu/environments/aroma/plugins/Inkay-openpak.wps", "MINE");   // somebody's own file under our name
    assert(Backup::stage(e, 200) == 2); assert(Backup::stage(e, 200) == 0);
    assert(get("/wiiu/environments/aroma/plugins/Inkay-openpak.wps") == "MINE");
    assert(!Backup::installed() && !Backup::wasInstalled());
    assert(Backup::install(e, 200)); assert(Backup::installed());
    assert(get("/wiiu/environments/aroma/modules/Inkay-openpak.wms") == "MOD1");
    assert(get("/wiiu/openpak/system/plugin.original") == "MINE");
    assert(get("/wiiu/openpak/system/module.absent") == "");
    assert(Backup::install(e, 200)); assert(get("/wiiu/openpak/system/plugin.original") == "MINE");
    assert(Backup::remove(e, 200));
    assert(get("/wiiu/environments/aroma/plugins/Inkay-openpak.wps") == "MINE");
    assert(get("/wiiu/environments/aroma/modules/Inkay-openpak.wms") == "<none>");
    assert(get("/wiiu/openpak/system/plugin.original") == "<none>" && !Backup::wasInstalled());
    // an update: the app carries a newer module
    assert(Backup::install(e, 200));
    unsigned char mod2[] = "MOD2"; items[0].data = mod2;
    assert(Backup::stage(e, 200) == 1); assert(Backup::wasInstalled()); assert(!Backup::installed());
    assert(Backup::install(e, 200)); assert(get("/wiiu/environments/aroma/modules/Inkay-openpak.wms") == "MOD2");
    // somebody else changed our file: kept, not removed
    put("/wiiu/environments/aroma/modules/Inkay-openpak.wms", "EDITED");
    assert(!Backup::remove(e, 200)); assert(strstr(e, "System files changed"));
    assert(get("/wiiu/environments/aroma/modules/Inkay-openpak.wms") == "EDITED");
    // a file under OpenPak's own name is an older OpenPak build: kept aside, never "restored"
    put("/wiiu/environments/aroma/modules/Inkay-openpak.wms", "MOD2");  // back to what OpenPak wrote
    assert(Backup::install(e, 200));
    assert(Backup::remove(e, 200));
    put("/wiiu/environments/aroma/modules/Inkay-openpak.wms", "OLDBUILD");
    items[0].own = true;
    assert(Backup::install(e, 200)); assert(get("/wiiu/openpak/system/module.replaced") == "OLDBUILD");
    assert(Backup::remove(e, 200)); assert(get("/wiiu/environments/aroma/modules/Inkay-openpak.wms") == "<none>");
    puts("ok");
}
