// JNI bridge between Kotlin NativeBridge and MobileDumper-7.
//
// MobileDumper-7 was originally a stand-alone executable. We invoke its
// Generator pipeline programmatically. Build flag DUMPER_BUILD_LIBRARY=ON
// in its CMake produces libMobileDumper-7.a which we link here.

#include <jni.h>
#include <android/log.h>
#include <string>
#include <sstream>
#include <mutex>

// MobileDumper-7 public API (headers from its Dumper/ subtree)
#include "MobileDumper-7/Dumper/Generator/Generators/Generator.h"
#include "MobileDumper-7/Dumper/Globals/Globals.h"
#include "MobileDumper-7/Dumper/Logger/Logger.h"
#include "MobileDumper-7/Dumper/Profile/ProfileManager.h"
#include "MobileDumper-7/Dumper/Memory/MemoryKitty.h"

#define LOG_TAG "mdumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {
    std::mutex       g_mutex;
    std::string      g_lastLog;
    std::string      g_lastError;

    // Capture MD7 logger output into g_lastLog.
    void capture_log_line(const std::string& line) {
        std::lock_guard<std::mutex> lk(g_mutex);
        g_lastLog += line;
        if (!line.empty() && line.back() != '\n') g_lastLog += '\n';
    }
}

extern "C" JNIEXPORT jint JNICALL
Java_com_dj_mdumper_NativeBridge_dump(
    JNIEnv* env, jobject /*thiz*/,
    jint pid, jstring joutDir, jstring jgameId)
{
    const char* outDir   = env->GetStringUTFChars(joutDir, nullptr);
    const char* gameId   = env->GetStringUTFChars(jgameId, nullptr);

    {
        std::lock_guard<std::mutex> lk(g_mutex);
        g_lastLog.clear();
        g_lastError.clear();
    }

    LOGI("dump(pid=%d, outDir=%s, gameId=%s)", pid, outDir, gameId);

    int rc = 0;
    try {
        // Install MD7 logger sink -> our capture buffer
        MD7::Globals::GLogger.SetSink([](auto level, const std::string& msg) {
            capture_log_line(msg);
        });

        // Attach via process_vm_readv (MD7 KittyMemoryEx backend)
        auto mem = std::make_unique<MD7::MemoryKitty>(
            static_cast<pid_t>(pid),
            MD7::MemoryKitty::ReadMode::ProcessVmReadv);
        if (!mem->IsAttached()) {
            g_lastError = "process_vm_readv failed (need root)";
            rc = 1;
            goto done;
        }
        MD7::Globals::GMemory = std::move(mem);

        // Pick profile. Empty = auto-detect by cmdline/module scan.
        std::string profileName = gameId;
        if (!profileName.empty()) {
            if (!MD7::ProfileManager::SelectByIdentifier(profileName)) {
                g_lastError = "no profile for " + profileName;
                rc = 2;
                goto done;
            }
        }

        // Output directory
        MD7::Globals::GSettings.General.OutputDirectory = outDir;

        // Run the generator — this does: InitUnrealModule, InitObjects, InitNames,
        //   InitSettings, InitOffsets, InitManagers, Generate dumps, Zip.
        MD7::Generator gen;
        std::string err;
        if (!gen.Run(err)) {
            g_lastError = "generator: " + err;
            rc = 3;
            goto done;
        }
    } catch (const std::exception& e) {
        g_lastError = std::string("exception: ") + e.what();
        rc = 4;
    } catch (...) {
        g_lastError = "unknown exception";
        rc = 5;
    }

done:
    env->ReleaseStringUTFChars(joutDir, outDir);
    env->ReleaseStringUTFChars(jgameId, gameId);
    LOGI("dump rc=%d err=%s", rc, g_lastError.c_str());
    return rc;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_dj_mdumper_NativeBridge_getLastLog(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lk(g_mutex);
    return env->NewStringUTF(g_lastLog.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_dj_mdumper_NativeBridge_getLastError(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lk(g_mutex);
    return env->NewStringUTF(g_lastError.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_dj_mdumper_NativeBridge_getBuildInfo(JNIEnv* env, jobject) {
    std::ostringstream os;
    os << "arm64-v8a, built " << __DATE__ << " " << __TIME__
       << ", MD7 bundled";
    return env->NewStringUTF(os.str().c_str());
}
