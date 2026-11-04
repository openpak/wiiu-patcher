#pragma once
// The Aroma files OpenPak installs on the SD card. Same rules as the Switch Patcher:
// before OpenPak replaces a file, the original is kept as <key>.original (or <key>.absent when
// there was none); OpenPak's version is kept as <key>.managed; selecting Nintendo puts the exact
// original back and removes what OpenPak added. A file changed by someone else is left alone.
#include <stddef.h>
#include <string>

namespace Backup {
    struct Item {
        const char *key;
        const char *target;          // relative to the SD root
        const unsigned char *data;   // the OpenPak version, carried inside the app
        size_t size;
        bool own;                    // the file name is OpenPak's own: an existing file is an older OpenPak build, kept as <key>.replaced and not restored
    };

    // Where the SD card is mounted (e.g. "fs:/vol/external01"); the state folder is <root>/wiiu/openpak/system.
    void init(const std::string &root, const Item *items, size_t count);
    // Copies the app's files into the managed copies when they differ. Returns how many changed, -1 on failure.
    int stage(char *err, size_t errlen);
    // Any target still holds an OpenPak file (current or from an earlier release).
    bool wasInstalled();
    // Every target holds the current OpenPak file.
    bool installed();
    int liveCount();
    // OpenPak selected / Nintendo selected.
    bool install(char *err, size_t errlen);
    bool remove(char *err, size_t errlen);
}
