// PUBG Mobile v4.7 walker — library form of pwnd_v6, for JNI callers.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <sys/types.h>

struct PubgFVector { float x, y, z; };

struct PubgEnemy {
    uintptr_t addr;
    float     hp;
    uint8_t   team;
    PubgFVector pos;
};

struct PubgDumpResult {
    bool        ok;
    std::string error;
    std::string log;
    int         pid;
    uintptr_t   libBase;
    uintptr_t   gObjects;
    uintptr_t   gNames;
    uintptr_t   processEvent;
    uintptr_t   world;
    uintptr_t   netDriver;
    size_t      numEnemies;
    uintptr_t   selfAddr;
};

/**
 * Run a one-shot dump against com.tencent.ig.
 *
 * @param outDir  writable directory where SDK.hpp, OffsetsInfo.json and
 *                enemies.json will be written
 * @return result with ok=true if dump reached the enemy scan stage
 */
PubgDumpResult pubg_dump(const std::string& outDir);
