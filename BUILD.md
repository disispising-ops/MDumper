# Building MDumper

## Option 1 — GitHub Actions (easiest, no local toolchain)

1. Push this repo to GitHub.
2. The included `.github/workflows/build.yml` builds the APK on every push.
3. Download the APK from the workflow's Artifacts.

No Android SDK/NDK/Gradle on your machine.

## Option 2 — Android Studio (desktop)

1. Install Android Studio Giraffe (2023.1) or newer.
2. `File → Open` → pick this directory.
3. Studio downloads Android SDK 34 + NDK 26 + Gradle 8.7 automatically on
   first sync.
4. Pull the MobileDumper-7 subtree:
   ```
   cd app/src/main/cpp
   git clone --recursive https://github.com/MJx0/MobileDumper-7.git
   ```
   (Or copy from your Downloads if you built it earlier in Termux.)
5. Apply the PUBG v4.7 UObject.Name patch to
   `MobileDumper-7/Dumper/Profile/CustomProfiles/Shared/PUBG.h` — see
   `patches/pubg_v4.7_uobject_name.patch`.
6. `Build → Build APK(s)`.

## Option 3 — Pure Termux build (on the phone, no desktop)

Works but is slow (~20 min on a fast phone). You need:

```
pkg install openjdk-17 gradle cmake ninja clang git
```

Then from the project root:

```
./gradlew assembleRelease
```

APK ends up in `app/build/outputs/apk/release/`.

Note: Termux Gradle pulls the Android SDK itself the first time and takes
~1.5 GB of disk.

## MobileDumper-7 subtree

The `app/src/main/cpp/MobileDumper-7/` directory is intentionally empty
in this repo. Clone it once:

```
git submodule add https://github.com/MJx0/MobileDumper-7.git app/src/main/cpp/MobileDumper-7
```

Or without submodules:

```
cd app/src/main/cpp && git clone --recursive https://github.com/MJx0/MobileDumper-7.git
```

Our CMake expects MD7 to expose a `MobileDumper-7` CMake target with
`DUMPER_BUILD_LIBRARY=ON`. If upstream changes its target name, update
`app/src/main/cpp/CMakeLists.txt`.
