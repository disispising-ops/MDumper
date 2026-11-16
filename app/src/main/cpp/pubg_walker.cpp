// PUBG Mobile v4.7 walker implementation — straight port of pwnd_v6.
#include "pubg_walker.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <dirent.h>
#include <sys/uio.h>
#include <unistd.h>

namespace OFF {
    constexpr uintptr_t GObjects       = 0x0F9AE670;
    constexpr uintptr_t GNames         = 0x0F6CA2D0;
    constexpr uintptr_t ProcessEvent   = 0x09290164;
    constexpr uintptr_t GNamesNested   = 0x110;
    constexpr uintptr_t FFOA_Objects   = 0x10;
    constexpr uintptr_t FFOA_NumObj    = 0x1C;
    constexpr uintptr_t FUOI_Size      = 0x18;
    constexpr uintptr_t UO_Class       = 0x10;
    constexpr uintptr_t UO_Name        = 0x18;
    constexpr uintptr_t World_Levels       = 0x0440;
    constexpr uintptr_t World_NetDriver    = 0x0038;
    constexpr uintptr_t Level_Actors       = 0x00A0;
    constexpr uintptr_t Char_Health        = 0x0E70;
    constexpr uintptr_t Char_TeamNum       = 0x1070;
    constexpr uintptr_t Pawn_Position      = 0x0128;
}

struct TArrayHeader { uintptr_t Data; int32_t Num; int32_t Max; };

struct ProcMem {
    pid_t pid = 0;
    bool Read(uintptr_t addr, void* out, size_t n) const {
        iovec local  { out, n };
        iovec remote { (void*)addr, n };
        return process_vm_readv(pid, &local, 1, &remote, 1, 0) == (ssize_t)n;
    }
    template<typename T> T R(uintptr_t addr) const {
        T v{}; Read(addr, &v, sizeof(T)); return v;
    }
};

static pid_t find_pid(const std::string& name) {
    DIR* d = opendir("/proc"); if (!d) return 0;
    pid_t found = 0;
    while (auto* e = readdir(d)) {
        if (e->d_type != DT_DIR) continue;
        pid_t pid = atoi(e->d_name); if (pid <= 0) continue;
        char path[64]; std::snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        std::ifstream f(path); if (!f) continue;
        std::string cmd; std::getline(f, cmd, '\0');
        if (cmd == name) { found = pid; break; }
    }
    closedir(d); return found;
}

static uintptr_t find_lib_base(pid_t pid, const std::string& lib) {
    char path[64]; std::snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    std::ifstream f(path); if (!f) return 0;
    std::string line; uintptr_t best = 0;
    while (std::getline(f, line)) {
        if (line.find(lib) == std::string::npos) continue;
        unsigned long long start, end, off; char perms[8];
        if (std::sscanf(line.c_str(), "%llx-%llx %7s %llx",
                        &start, &end, perms, &off) != 4) continue;
        uintptr_t base = (uintptr_t)start - (uintptr_t)off;
        if (best == 0 || base < best) best = base;
    }
    return best;
}

constexpr int kNamesPerChunk = 0x4000;

static uintptr_t resolve_gnames_array(const ProcMem& mem, uintptr_t libBase) {
    uintptr_t mgr = mem.R<uintptr_t>(libBase + OFF::GNames); if (!mgr) return 0;
    uintptr_t arr = mem.R<uintptr_t>(mgr + OFF::GNamesNested); if (!arr) return 0;
    return mem.R<uintptr_t>(arr);
}

static uintptr_t get_name_entry(const ProcMem& mem, uintptr_t chunkTable, int32_t compIdx) {
    if (!chunkTable || compIdx < 0) return 0;
    int chunkIdx = compIdx / kNamesPerChunk;
    int inChunk  = compIdx % kNamesPerChunk;
    uintptr_t chunk = mem.R<uintptr_t>(chunkTable + chunkIdx * sizeof(uintptr_t));
    if (!chunk) return 0;
    return mem.R<uintptr_t>(chunk + inChunk * sizeof(uintptr_t));
}

static std::string read_fname_entry(const ProcMem& mem, uintptr_t entry) {
    if (!entry) return {};
    int32_t idx = mem.R<int32_t>(entry + 0x08);
    bool isWide = (idx & 1) != 0;
    uintptr_t strAddr = entry + 0x0C;
    if (isWide) {
        char16_t buf[128]{};
        mem.Read(strAddr, buf, sizeof(buf) - sizeof(char16_t));
        std::string out;
        for (int i = 0; i < 127 && buf[i]; ++i) out += (char)(buf[i] & 0x7F);
        return out;
    }
    char buf[128]{};
    mem.Read(strAddr, buf, sizeof(buf) - 1);
    return std::string(buf);
}

static std::string obj_class_name(const ProcMem& mem, uintptr_t obj, uintptr_t chunkTable) {
    uintptr_t cls = mem.R<uintptr_t>(obj + OFF::UO_Class); if (!cls) return {};
    int32_t compIdx = mem.R<int32_t>(cls + OFF::UO_Name) & ~1;
    uintptr_t entry = get_name_entry(mem, chunkTable, compIdx);
    return read_fname_entry(mem, entry);
}

static bool is_heap_ptr(uintptr_t p) {
    return p >= 0x7000000000ULL && p <= 0x7FFFFFFFFFULL;
}

static uintptr_t find_world(const ProcMem& mem, uintptr_t gObjAddr,
                             uintptr_t chunkTable, uintptr_t libBase) {
    uintptr_t objectsArr = mem.R<uintptr_t>(gObjAddr + OFF::FFOA_Objects);
    int32_t numObjects = mem.R<int32_t>(gObjAddr + OFF::FFOA_NumObj);
    // Looser check: any object with a Levels TArray pointing to a ULevel that
    // has Actors.Num > 100 and a NetDriver pointer in its own body.
    for (int32_t i = 0; i < numObjects; ++i) {
        uintptr_t obj = mem.R<uintptr_t>(objectsArr + (uintptr_t)i * OFF::FUOI_Size);
        if (!is_heap_ptr(obj)) continue;
        TArrayHeader h = mem.R<TArrayHeader>(obj + OFF::World_Levels);
        if (h.Num <= 0 || h.Num > 100) continue;
        if (!is_heap_ptr(h.Data)) continue;
        uintptr_t lvl0 = mem.R<uintptr_t>(h.Data);
        if (!is_heap_ptr(lvl0)) continue;
        // Level vtable in exec range
        uintptr_t lvlVtbl = mem.R<uintptr_t>(lvl0);
        if (lvlVtbl < libBase || lvlVtbl > libBase + 0x20000000) continue;
        TArrayHeader a = mem.R<TArrayHeader>(lvl0 + OFF::Level_Actors);
        if (a.Num < 100 || a.Num > 500000) continue;
        if (!is_heap_ptr(a.Data)) continue;
        // World should have a NetDriver pointer at +0x38
        uintptr_t nd = mem.R<uintptr_t>(obj + OFF::World_NetDriver);
        if (!is_heap_ptr(nd)) continue;
        uintptr_t ndVtbl = mem.R<uintptr_t>(nd);
        if (ndVtbl < libBase || ndVtbl > libBase + 0x20000000) continue;
        return obj;
    }
    return 0;
}

PubgDumpResult pubg_dump(const std::string& outDir) {
    PubgDumpResult r{};
    std::ostringstream log;

    auto logf = [&](const std::string& s){ log << s << "\n"; };

    r.pid = find_pid("com.tencent.ig");
    if (!r.pid) { r.error = "PUBG not running"; r.log = log.str(); return r; }
    logf("[+] PID: " + std::to_string(r.pid));

    ProcMem mem{r.pid};
    r.libBase = find_lib_base(r.pid, "libUE4.so");
    if (!r.libBase) { r.error = "libUE4.so not mapped"; r.log = log.str(); return r; }
    {
        char b[64]; std::snprintf(b, sizeof(b), "[+] libUE4 base: 0x%lx", (unsigned long)r.libBase);
        logf(b);
    }

    r.gObjects     = r.libBase + OFF::GObjects;
    r.gNames       = r.libBase + OFF::GNames;
    r.processEvent = r.libBase + OFF::ProcessEvent;

    uintptr_t chunkTable = resolve_gnames_array(mem, r.libBase);
    if (!chunkTable) { r.error = "GNames resolve failed"; r.log = log.str(); return r; }
    logf("[+] GNames resolved");

    r.world = find_world(mem, r.gObjects, chunkTable, r.libBase);
    if (!r.world) { r.error = "UWorld not findable (in match?)"; r.log = log.str(); return r; }
    {
        char b[64]; std::snprintf(b, sizeof(b), "[+] UWorld: 0x%lx", (unsigned long)r.world);
        logf(b);
    }

    r.netDriver = mem.R<uintptr_t>(r.world + OFF::World_NetDriver);
    if (!is_heap_ptr(r.netDriver)) { r.error = "NetDriver not findable"; r.log = log.str(); return r; }
    {
        char b[64]; std::snprintf(b, sizeof(b), "[+] NetDriver: 0x%lx", (unsigned long)r.netDriver);
        logf(b);
    }

    // Deep walk for pawns
    std::vector<uintptr_t> maybeConns;
    for (uintptr_t off = 0x40; off <= 0x400; off += 8) {
        uintptr_t p = mem.R<uintptr_t>(r.netDriver + off);
        if (!is_heap_ptr(p)) continue;
        if (p == r.world || p == r.netDriver || p == r.netDriver + 0x28) continue;
        uintptr_t vtbl = mem.R<uintptr_t>(p);
        if (vtbl < r.libBase || vtbl > r.libBase + 0x20000000) continue;
        maybeConns.push_back(p);
    }
    logf("[+] " + std::to_string(maybeConns.size()) + " conn candidates");

    std::vector<PubgEnemy> enemies;
    std::unordered_map<uintptr_t,int> refCount;
    std::unordered_set<uintptr_t> seen;
    for (uintptr_t conn : maybeConns) {
        for (uintptr_t cOff = 0x60; cOff <= 0x400; cOff += 8) {
            TArrayHeader h = mem.R<TArrayHeader>(conn + cOff);
            if (h.Num <= 0 || h.Num > 5000) continue;
            if (!is_heap_ptr(h.Data)) continue;
            int cap = h.Num > 300 ? 300 : h.Num;
            for (int i = 0; i < cap; ++i) {
                uintptr_t entry = mem.R<uintptr_t>(h.Data + i * sizeof(uintptr_t));
                if (!is_heap_ptr(entry)) continue;
                for (uintptr_t eOff = 0x40; eOff <= 0x180; eOff += 8) {
                    uintptr_t cand = mem.R<uintptr_t>(entry + eOff);
                    if (!is_heap_ptr(cand)) continue;
                    std::string cn = obj_class_name(mem, cand, chunkTable);
                    if (cn != "BP_PlayerPawn_C") continue;
                    refCount[cand]++;
                    if (seen.count(cand)) continue;
                    seen.insert(cand);
                    PubgEnemy e;
                    e.addr = cand;
                    e.hp   = mem.R<float>(cand + OFF::Char_Health);
                    e.team = mem.R<uint8_t>(cand + OFF::Char_TeamNum);
                    e.pos  = mem.R<PubgFVector>(cand + OFF::Pawn_Position);
                    if (e.hp > 0 && e.hp <= 300 &&
                        (std::fabs(e.pos.x) + std::fabs(e.pos.y) > 1)) {
                        enemies.push_back(e);
                    }
                }
            }
        }
    }
    r.numEnemies = enemies.size();

    uintptr_t selfAddr = 0; int maxRefs = 0;
    for (auto& [a, c] : refCount) if (c > maxRefs) { maxRefs = c; selfAddr = a; }
    r.selfAddr = selfAddr;

    // Write offsets.json
    {
        std::ofstream f(outDir + "/OffsetsInfo.json");
        f << "{\n";
        f << "  \"credit\":{\"tool\":\"MDumper\",\"version\":\"0.2.0\"},\n";
        f << "  \"libBase\":  \"0x" << std::hex << r.libBase << std::dec << "\",\n";
        f << "  \"GObjects_RVA\": \"0x" << std::hex << OFF::GObjects << std::dec << "\",\n";
        f << "  \"GNames_RVA\":   \"0x" << std::hex << OFF::GNames << std::dec << "\",\n";
        f << "  \"ProcessEvent_RVA\": \"0x" << std::hex << OFF::ProcessEvent << std::dec << "\",\n";
        f << "  \"UObject.Class\": \"0x10\", \"UObject.Name\": \"0x18\",\n";
        f << "  \"Char.Health\":  \"0xE70\",\n";
        f << "  \"Char.TeamNum\": \"0x1070\",\n";
        f << "  \"Pawn.Position\":\"0x128\",\n";
        f << "  \"UWorld.Levels\":   \"0x440\",\n";
        f << "  \"UWorld.NetDriver\":\"0x038\",\n";
        f << "  \"ULevel.Actors\":   \"0x0A0\"\n";
        f << "}\n";
    }

    // Write enemies.json for debugging/consumers
    {
        std::ofstream f(outDir + "/enemies.json");
        f << "{\n  \"numEnemies\": " << enemies.size() << ",\n";
        f << "  \"self\": \"0x" << std::hex << selfAddr << std::dec << "\",\n";
        f << "  \"enemies\": [\n";
        for (size_t i = 0; i < enemies.size(); ++i) {
            auto& e = enemies[i];
            f << "    {\"addr\":\"0x" << std::hex << e.addr << std::dec
              << "\",\"hp\":" << e.hp
              << ",\"team\":" << (int)e.team
              << ",\"pos\":[" << e.pos.x << "," << e.pos.y << "," << e.pos.z << "]}";
            if (i + 1 < enemies.size()) f << ",";
            f << "\n";
        }
        f << "  ]\n}\n";
    }

    // Minimal SDK.hpp stub — placeholder until full SDK generator is ported
    {
        std::ofstream f(outDir + "/SDK.hpp");
        f << "// MDumper 0.2.0 — SDK stub for PUBG Mobile v4.7 (com.tencent.ig)\n";
        f << "// Full SDK generator pending; offsets already in OffsetsInfo.json.\n\n";
        f << "#pragma once\n#include <cstdint>\nnamespace SDK {\n";
        f << "constexpr uintptr_t GObjects_RVA     = 0x" << std::hex << OFF::GObjects << ";\n";
        f << "constexpr uintptr_t GNames_RVA       = 0x" << OFF::GNames << ";\n";
        f << "constexpr uintptr_t ProcessEvent_RVA = 0x" << OFF::ProcessEvent << ";\n" << std::dec;
        f << "struct FVector { float x, y, z; };\n";
        f << "struct ASTExtraCharacter {\n";
        f << "    // ... (parent fields elided)\n";
        f << "    float   Health;        // +0x0E70\n";
        f << "    float   HealthMax;     // +0x0E74\n";
        f << "    uint8_t TeamNum;       // +0x1070\n";
        f << "    FVector Position;      // +0x0128 (cached world pos)\n";
        f << "};\n";
        f << "} // namespace SDK\n";
    }

    logf("[+] " + std::to_string(enemies.size()) + " enemies found");
    logf("[+] Wrote OffsetsInfo.json, enemies.json, SDK.hpp");

    r.ok = true;
    r.log = log.str();
    return r;
}
