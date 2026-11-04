// OpenPak Wii U Patcher: flips a Wii U between Nintendo's servers and OpenPak's.
//
// A .wuhb app for Aroma. It carries the Inkay-openpak module and plugin, installs them into
// sd:/wiiu/environments/aroma (keeping whatever it replaces as a backup), and offers the reboot
// that makes them load. Selecting Nintendo takes everything back out. Same screen, strings and
// rules as the OpenPak Switch Patcher.
#include <SDL.h>
#include <SDL2_gfxPrimitives.h>
#include <SDL_ttf.h>

#include <coreinit/launch.h>
#include <sysapp/launch.h>
#include <vpad/input.h>
#include <whb/sdcard.h>

#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <string>

#include "Backup.hpp"
#include "redirect_list.h"
#include "font_bold_ttf.h"
#include "font_regular_ttf.h"
#include "module_wms.h"
#include "plugin_wps.h"

#define APP_VERSION "0.6.0"

namespace {
// openpak.org's dark theme, as in the Switch Patcher.
struct Rgb { Uint8 r, g, b; };
constexpr Rgb PAPER{0x10, 0x11, 0x14}, RAISED{0x1c, 0x1e, 0x24}, SELECT{0x23, 0x2a, 0x3d}, LINE{0x26, 0x28, 0x2e},
    INK{0xe9, 0xea, 0xee}, STRONG{0xff, 0xff, 0xff}, MUTED{0x8a, 0x8e, 0x99}, ACCENT{0x2f, 0x56, 0xd0},
    OK{0x3d, 0xc0, 0x7c}, OK_BG{0x14, 0x2c, 0x20}, WARN{0xe0, 0xa8, 0x50}, WARN_BG{0x2c, 0x24, 0x14};

constexpr int W = 1280, H = 720, MARGIN = 72;
enum { IT_NINTENDO, IT_OPENPAK, IT_COUNT };
const char *const itemLabels[IT_COUNT] = {"Nintendo", "OpenPak"};

SDL_Renderer *rnd = nullptr;
// Pixel sizes follow the Switch Patcher: display 46, title 34, body 24, label 19.
enum { PX_TITLE = 34, PX_BODY = 24, PX_LABEL = 19 };
TTF_Font *fontTitle, *fontBody, *fontLabel;

std::string sdRoot;
const std::string AROMA = "wiiu/environments/aroma";

Backup::Item items[2];

// ---- drawing --------------------------------------------------------------------------------
void fill(Rgb c) { SDL_SetRenderDrawColor(rnd, c.r, c.g, c.b, 255); }
void rect(int x, int y, int w, int h, Rgb c) { fill(c); SDL_Rect r{x, y, w, h}; SDL_RenderFillRect(rnd, &r); }
void rounded(int x, int y, int w, int h, int r, Rgb c) { roundedBoxRGBA(rnd, x, y, x + w - 1, y + h - 1, r, c.r, c.g, c.b, 255); }
void outline(int x, int y, int w, int h, int r, Rgb c) { roundedRectangleRGBA(rnd, x, y, x + w - 1, y + h - 1, r, c.r, c.g, c.b, 255); }
void circle(int x, int y, int r, Rgb c) { filledCircleRGBA(rnd, x, y, r, c.r, c.g, c.b, 255); }

enum Align { LEFT, CENTER, RIGHT };
TTF_Font *fontFor(int px) { return px == PX_TITLE ? fontTitle : px == PX_BODY ? fontBody : fontLabel; }

int text(int x, int y, int px, Align a, Rgb c, const char *fmt, ...) {
    char buf[300];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
    if (!buf[0]) return 0;
    SDL_Surface *s = TTF_RenderUTF8_Blended(fontFor(px), buf, SDL_Color{c.r, c.g, c.b, 255});
    if (!s) return 0;
    SDL_Texture *t = SDL_CreateTextureFromSurface(rnd, s);
    int w = s->w, h = s->h;
    SDL_Rect d{a == CENTER ? x - w / 2 : a == RIGHT ? x - w : x, y, w, h};
    SDL_FreeSurface(s);
    if (t) { SDL_RenderCopy(rnd, t, nullptr, &d); SDL_DestroyTexture(t); }
    return w;
}

int textWidth(int px, const char *s) {
    int w = 0, h = 0;
    TTF_SizeUTF8(fontFor(px), s, &w, &h);
    return w;
}

// The OpenPak mark: a ring, an opening ring and a dot.
void mark(int x, int y, int size) {
    int cx = x + size / 2, cy = y + size / 2;
    float s = size / 512.0f;
    for (int i = 0; i < 4; i++) aacircleRGBA(rnd, cx, cy, (int)(180 * s) - i, 255, 255, 255, 255);
    for (int i = 0; i < 4; i++) arcRGBA(rnd, cx, cy, (int)(96 * s) - i, 90, 270, 255, 255, 255, 255);
    filledCircleRGBA(rnd, cx, cy, (int)(26 * s), 255, 255, 255, 255);
}

int hint(int x, int y, const char *glyph, const char *caption) {
    const int r = 18;
    circle(x + r, y + r, r, RAISED);
    text(x + r, y + r - PX_LABEL / 2 - 4, PX_LABEL, CENTER, INK, "%s", glyph);
    int w = text(x + r * 2 + 12, y + r - PX_BODY / 2 - 4, PX_BODY, LEFT, MUTED, "%s", caption);
    return r * 2 + 12 + w + 38;
}

// ---- system state ---------------------------------------------------------------------------
bool exists(const std::string &p) { struct stat st; return stat(p.c_str(), &st) == 0; }

std::string configPath() { return sdRoot + "/" + AROMA + "/plugins/config/inkay.json"; }

std::string slurp(const std::string &path, bool *ok) {
    std::string s;
    FILE *f = fopen(path.c_str(), "rb");
    *ok = f != nullptr;
    if (!f) return s;
    char b[512]; size_t n;
    while ((n = fread(b, 1, sizeof(b), f)) > 0) s.append(b, n);
    fclose(f);
    return s;
}

// connect_to_network in Inkay's settings file; a missing file or key means the default, on.
bool readFlag() {
    bool ok;
    std::string s = slurp(configPath(), &ok);
    size_t k = s.find("\"connect_to_network\"");
    if (!ok || k == std::string::npos) return true;
    size_t colon = s.find(':', k);
    if (colon == std::string::npos) return true;
    size_t v = s.find_first_not_of(" \t\r\n", colon + 1);
    return !(v != std::string::npos && s.compare(v, 5, "false") == 0);
}

// Rewrites only that key, through a temp file and a rename like the backups.
bool writeFlag(bool value) {
    bool ok;
    std::string path = configPath(), s = slurp(path, &ok);
    if (!ok) return true;   // no settings file: the default stands
    size_t k = s.find("\"connect_to_network\"");
    if (k == std::string::npos) return true;
    size_t colon = s.find(':', k);
    if (colon == std::string::npos) return true;
    size_t v = s.find_first_not_of(" \t\r\n", colon + 1);
    if (v == std::string::npos) return true;
    size_t e = s.compare(v, 5, "false") == 0 ? v + 5 : v + 4;
    s.replace(v, e - v, value ? "true" : "false");
    std::string tmp = path + ".openpak-tmp";
    FILE *o = fopen(tmp.c_str(), "wb");
    if (!o) return false;
    bool good = fwrite(s.data(), 1, s.size(), o) == s.size();
    good = fclose(o) == 0 && good;
    if (!good) { std::remove(tmp.c_str()); return false; }
    std::remove(path.c_str());
    return std::rename(tmp.c_str(), path.c_str()) == 0;
}

std::string flagMarker() { return sdRoot + "/wiiu/openpak/system/config.flag-was-off"; }

// OpenPak turns the plugin's own switch on if the player had it off, and remembers that, so
// selecting Nintendo can put it back as it was.
bool enableFlag(char *err, size_t n) {
    if (readFlag()) return true;
    FILE *m = fopen(flagMarker().c_str(), "wb");
    if (m) fclose(m);
    if (!writeFlag(true)) { snprintf(err, n, "Could not update the Inkay settings"); return false; }
    return true;
}

bool restoreFlag(char *err, size_t n) {
    if (!exists(flagMarker())) return true;
    if (!writeFlag(false)) { snprintf(err, n, "Could not restore the Inkay settings"); return false; }
    std::remove(flagMarker().c_str());
    return true;
}

bool endsWith(const std::string &s, const char *suffix) {
    size_t n = strlen(suffix);
    return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
}

// Another Inkay (Pretendo's own) loads under the same module name: two cannot run together.
std::string foreignInkay() {
    for (const char *dir : {"modules", "plugins"}) {
        DIR *dp = opendir((sdRoot + "/" + AROMA + "/" + dir).c_str());
        if (!dp) continue;
        while (dirent *e = readdir(dp)) {
            std::string n = e->d_name;
            bool inkay = n.rfind("Inkay", 0) == 0 && (endsWith(n, ".wms") || endsWith(n, ".wps"));
            if (inkay && n.rfind("Inkay-openpak.", 0) != 0) { closedir(dp); return n; }
        }
        closedir(dp);
    }
    return "";
}

// "Redirects: N names (signed list vX)": the list the plugin cached on the SD card, verified here
// with the same pinned key; with no valid cache, the bundled list.
void redirectStatus(char *out, size_t n) {
    RedirectList l = redirect_bundled();
    bool ok;
    std::string env = slurp(sdRoot + "/wiiu/openpak/redirects.json", &ok), err;
    if (ok && redirect_parse_envelope(env.data(), env.size(), &l, &err))
        snprintf(out, n, "Redirects: %zu names (signed list v%lld)", l.count(), (long long) l.version);
    else
        snprintf(out, n, "Redirects: %zu names (bundled list)", l.count());
}

bool isOn() { return Backup::installed() && readFlag(); }

void reboot() {
    WHBUnmountSdCard();
    OSLaunchTitlel(OS_TITLE_ID_REBOOT, 0);
    // Still here: the reboot was not accepted. Relaunch the menu; Aroma loads modules at power on.
    OSForceFullRelaunch();
    SYSLaunchMenu();
}

// ---- screen ---------------------------------------------------------------------------------
struct Ui {
    bool on = false, confirm = false;
    int sel = IT_NINTENDO;
    char status[200] = "";
    char note[200] = "";
    char redirects[120] = "";
    std::string foreign;
} ui;

void render() {
    const int card_w = W - MARGIN * 2;
    fill(PAPER); SDL_RenderClear(rnd);

    mark(MARGIN, 48, 56);
    text(MARGIN + 74, 44, PX_TITLE, LEFT, STRONG, "OpenPak Wii U Patcher");
    text(MARGIN + 76, 92, PX_LABEL, LEFT, MUTED, "Network selector");

    const char *pill = ui.on ? "OPENPAK" : "NINTENDO";
    int pill_w = textWidth(PX_LABEL, pill) + 40;
    rounded(W - MARGIN - pill_w, 58, pill_w, 40, 20, ui.on ? OK_BG : WARN_BG);
    outline(W - MARGIN - pill_w, 58, pill_w, 40, 20, ui.on ? OK : WARN);
    text(W - MARGIN - pill_w / 2, 64, PX_LABEL, CENTER, ui.on ? OK : WARN, "%s", pill);
    rect(MARGIN, 132, card_w, 1, LINE);

    const int row_h = 72, card_y = 164, card_h = row_h * IT_COUNT + 24;
    rounded(MARGIN, card_y, card_w, card_h, 18, RAISED);
    outline(MARGIN, card_y, card_w, card_h, 18, LINE);
    for (int i = 0; i < IT_COUNT; i++) {
        bool active = (i == IT_OPENPAK) == ui.on;
        int y = card_y + 12 + i * row_h, x = MARGIN + 12, w = card_w - 24;
        if (i == ui.sel) { rounded(x, y, w, row_h, 12, SELECT); rounded(x, y + 10, 4, row_h - 20, 2, ACCENT); }
        text(x + 26, y + row_h / 2 - PX_BODY / 2 - 6, PX_BODY, LEFT, i == ui.sel ? STRONG : INK, "%s", itemLabels[i]);
        if (active) text(x + w - 26, y + row_h / 2 - PX_BODY / 2 - 6, PX_BODY, RIGHT, ui.on ? OK : WARN, "Active");
        if (i + 1 < IT_COUNT) rect(MARGIN + 38, y + row_h, card_w - 76, 1, LINE);
    }

    int msg_y = card_y + card_h + 26;
    if (ui.status[0]) {
        rounded(MARGIN, msg_y, card_w, 56, 12, RAISED);
        outline(MARGIN, msg_y, card_w, 56, 12, ui.confirm ? WARN : ACCENT);
        text(MARGIN + 24, msg_y + 12, PX_BODY, LEFT, INK, "%s", ui.status);
    } else {
        text(MARGIN, msg_y + 12, PX_LABEL, LEFT, MUTED, "Changes apply at boot: Aroma loads the module and plugin then.");
        text(MARGIN, msg_y + 38, PX_LABEL, LEFT, MUTED, "%d of 2 OpenPak files installed", Backup::liveCount());
        text(MARGIN, msg_y + 64, PX_LABEL, LEFT, MUTED, "%s", ui.redirects);
    }
    int ny = msg_y + (ui.status[0] ? 76 : 90);
    if (ui.note[0]) { text(MARGIN, ny, PX_LABEL, LEFT, WARN, "%s", ui.note); ny += 26; }
    if (!ui.foreign.empty())
        text(MARGIN, ny, PX_LABEL, LEFT, WARN, "Another Inkay is installed (%s): remove it first.", ui.foreign.c_str());

    rect(MARGIN, 604, card_w, 1, LINE);
    int x = MARGIN;
    if (ui.confirm) { x += hint(x, 628, "A", "Reboot now"); hint(x, 628, "B", "Later"); }
    else { x += hint(x, 628, "A", "Select"); hint(x, 628, "B", "Exit"); }
    text(W - MARGIN, 636, PX_LABEL, RIGHT, MUTED, "v" APP_VERSION);
    SDL_RenderPresent(rnd);
}

void select(bool wantOpenPak) {
    char err[160] = "";
    bool again = wantOpenPak == ui.on;
    ui.note[0] = '\0';
    if (!exists(sdRoot + "/" + AROMA)) {
        snprintf(ui.status, sizeof(ui.status), "Failed: Aroma was not found on the SD card");
        return;
    }
    bool ok = wantOpenPak ? (Backup::install(err, sizeof(err)) && enableFlag(err, sizeof(err)))
                          : (Backup::remove(err, sizeof(err)) && restoreFlag(err, sizeof(err)));
    if (!ok) { snprintf(ui.status, sizeof(ui.status), "Failed: %s", err); return; }
    ui.on = wantOpenPak;
    ui.confirm = true;
    snprintf(ui.status, sizeof(ui.status), again ? "%s setup updated. Reboot to apply?" : "%s selected. Reboot to apply?",
             wantOpenPak ? "OpenPak" : "Nintendo");
    ui.foreign = foreignInkay();
}
}  // namespace

int main(int, char **) {
    WHBMountSdCard();
    const char *mp = WHBGetSdCardMountPath();
    sdRoot = mp ? mp : "fs:/vol/external01";
    items[0] = {"module", "wiiu/environments/aroma/modules/Inkay-openpak.wms", module_wms, module_wms_size, true};
    items[1] = {"plugin", "wiiu/environments/aroma/plugins/Inkay-openpak.wps", plugin_wps, plugin_wps_size, true};
    Backup::init(sdRoot, items, 2);

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window *win = SDL_CreateWindow("OpenPak Wii U Patcher", 0, 0, W, H, 0);
    rnd = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    fontTitle = TTF_OpenFontRW(SDL_RWFromConstMem(font_bold_ttf, font_bold_ttf_size), 1, PX_TITLE);
    fontBody = TTF_OpenFontRW(SDL_RWFromConstMem(font_regular_ttf, font_regular_ttf_size), 1, PX_BODY);
    fontLabel = TTF_OpenFontRW(SDL_RWFromConstMem(font_regular_ttf, font_regular_ttf_size), 1, PX_LABEL);
    VPADInit();

    // The files inside the app become the managed copies. A newer set replaces the one an earlier
    // release installed, and the reboot is offered.
    char err[160] = "";
    int changed = Backup::stage(err, sizeof(err));
    if (changed < 0) snprintf(ui.status, sizeof(ui.status), "Failed: %s", err);
    else if (changed > 0 && Backup::wasInstalled()) {
        if (Backup::install(err, sizeof(err))) { ui.confirm = true; snprintf(ui.status, sizeof(ui.status), "OpenPak setup updated. Reboot to apply?"); }
        else snprintf(ui.status, sizeof(ui.status), "Failed: %s", err);
    }
    redirectStatus(ui.redirects, sizeof(ui.redirects));
    ui.on = isOn();
    ui.sel = ui.on ? IT_OPENPAK : IT_NINTENDO;
    ui.foreign = foreignInkay();
    if (!exists(sdRoot + "/" + AROMA)) snprintf(ui.note, sizeof(ui.note), "Aroma was not found on the SD card.");

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) if (e.type == SDL_QUIT) running = false;

        VPADStatus st; VPADReadError verr;
        VPADRead(VPAD_CHAN_0, &st, 1, &verr);
        uint32_t down = verr == VPAD_READ_SUCCESS ? st.trigger : 0;
        if (ui.confirm) {
            if (down & VPAD_BUTTON_A) reboot();
            else if (down & VPAD_BUTTON_B) { ui.confirm = false; ui.status[0] = '\0'; }
        } else if (down & (VPAD_BUTTON_UP | VPAD_STICK_L_EMULATION_UP)) ui.sel = (ui.sel + IT_COUNT - 1) % IT_COUNT;
        else if (down & (VPAD_BUTTON_DOWN | VPAD_STICK_L_EMULATION_DOWN)) ui.sel = (ui.sel + 1) % IT_COUNT;
        else if (down & (VPAD_BUTTON_B | VPAD_BUTTON_PLUS)) running = false;
        else if (down & VPAD_BUTTON_A) select(ui.sel == IT_OPENPAK);
        render();
    }

    TTF_Quit();
    SDL_Quit();
    WHBUnmountSdCard();
    return 0;
}
