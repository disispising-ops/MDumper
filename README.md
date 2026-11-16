# MDumper — UE4 SDK Dumper for Android

On-device Unreal Engine 4 SDK dumper. Point it at a running UE4 game on a
rooted Android phone, press one button, get SDK headers + offsets you can
feed to any cheat-generation pipeline (including an LLM).

## Why

Every existing UE4 Android dumper (MobileDumper-7, AndUE4Dumper, Dumper-7…)
needs Termux + CMake + Ninja + clang + root commands, and a fresh build
every time the target game updates. MDumper packages that whole pipeline
into one tap on your phone.

## Supported targets

- **PUBG Mobile Global** (`com.tencent.ig`) — v4.7+ profile baked in
- **PUBG Mobile KR/JP** (`com.pubg.krmobile`)
- **BGMI** (`com.pubg.imobile`)
- **PUBG Vietnam** (`com.vng.pubgmobile`)
- **PUBG Taiwan** (`com.rekoo.pubgm`)
- **PUBG HD (CN)** (`com.tencent.tmgp.pubgmhd`)
- Any other UE4 game whose profile MobileDumper-7 supports — picker in UI

## Requirements

- Android 7+ (arm64-v8a only — same as PUBG itself)
- Root access via **KernelSU-Next**, Magisk, or SuperSU
- Target game installed and currently running

## Build

See `BUILD.md`. Three options: Android Studio, pure Termux, or GitHub Actions.

## Output

Dumps land in `/sdcard/Download/MDumper/<pkg>_<version>_<timestamp>/`:

```
CppSDK/              — SDK.hpp + per-module *_classes.hpp / *_structs.hpp
Dumpspace/           — OffsetsInfo.json, ClassesInfo.json, FunctionsInfo.json
Mappings/            — .usmap for client-side modders
IDAMappings/         — .idmap + IDA/Ghidra import scripts
GObjects-Dump.txt    — live UObject enumeration (class + addr + name)
```

Share the resulting `.zip` to any LLM that can work with C++ headers and
ask it to build whatever cheat you want — ESP, aimbot, fly, speed. MDumper
itself never writes to the target process.

## Credits

Wraps [MobileDumper-7](https://github.com/MJx0/MobileDumper-7) by MJx0
and contributors. PUBG profile includes the `UObject.Name = 0x18` patch
needed for v4.7+ which encrypts FName.CompIdx at that slot.

## License

MIT for the MDumper wrapper. MobileDumper-7 ships under its own license —
see `app/src/main/cpp/MobileDumper-7/LICENSE`.
