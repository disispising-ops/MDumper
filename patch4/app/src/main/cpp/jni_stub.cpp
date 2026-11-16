// Thin JNI exports for things NOT handled by MobileDumper-7 directly.
// MD7 exports Java_com_dj_mdumper_NativeBridge_dumpFull itself (patched in).
// We export the quick-walker and the metadata functions.
#include <jni.h>
#include <android/log.h>
#include <mutex>
#include <string>
#include "pubg_walker.h"

#define LOG_TAG "mdumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace {
    std::mutex       g_m;
    std::string      g_log;
    std::string      g_err;
}

// Quick walker — doesn't dump SDK, just reads ESP data via NetDriver.
// For backwards compat with the v0.2 UI button.
extern "C" JNIEXPORT jint JNICALL
Java_com_dj_mdumper_NativeBridge_dump(JNIEnv* env, jobject,
                                      jint pid, jstring joutDir, jstring jgameId) {
    const char* outDir = env->GetStringUTFChars(joutDir, nullptr);
    const char* gameId = env->GetStringUTFChars(jgameId, nullptr);
    LOGI("quick dump pid=%d gameId=%s", pid, gameId);
    int rc = 0;
    {
        std::lock_guard<std::mutex> lk(g_m);
        g_log.clear(); g_err.clear();
    }
    try {
        auto r = pubg_dump(outDir);
        std::lock_guard<std::mutex> lk(g_m);
        g_log = r.log;
        if (!r.ok) { g_err = r.error; rc = 1; }
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lk(g_m);
        g_err = std::string("exception: ") + e.what(); rc = 4;
    } catch (...) {
        std::lock_guard<std::mutex> lk(g_m);
        g_err = "unknown exception"; rc = 5;
    }
    env->ReleaseStringUTFChars(joutDir, outDir);
    env->ReleaseStringUTFChars(jgameId, gameId);
    return rc;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_dj_mdumper_NativeBridge_getLastLog(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lk(g_m);
    return env->NewStringUTF(g_log.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_dj_mdumper_NativeBridge_getLastError(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lk(g_m);
    return env->NewStringUTF(g_err.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_dj_mdumper_NativeBridge_getBuildInfo(JNIEnv* env, jobject) {
    std::string s = "MDumper 0.3.0 full-SDK, arm64-v8a, built " __DATE__ " " __TIME__;
    return env->NewStringUTF(s.c_str());
}
