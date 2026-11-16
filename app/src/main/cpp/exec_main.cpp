// Standalone executable entrypoint. Run via `su -c /path/to/libmdumper_exec.so <outDir>`
// The .so extension is cosmetic — Android APK extraction preserves executable
// bits on anything in nativeLibraryDir regardless of extension.

#include "pubg_walker.h"
#include <cstdio>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <output-dir>\n", argv[0]);
        return 2;
    }
    std::string outDir = argv[1];
    auto r = pubg_dump(outDir);
    std::fputs(r.log.c_str(), stdout);
    if (!r.ok) {
        std::fprintf(stderr, "ERROR: %s\n", r.error.c_str());
        return 1;
    }
    std::printf("[+] done. ok=%d enemies=%zu\n", r.ok ? 1 : 0, r.numEnemies);
    return 0;
}
