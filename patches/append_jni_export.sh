#!/bin/bash
# Append MDumper JNI export to MD7's main_android.cpp
set -e
F="Dumper/main_android.cpp"
# strip final #endif line
sed -i '/^#endif \/\/ __ANDROID__$/d' "$F"
cat >> "$F" <<'CPP'

// --- MDumper JNI entrypoint ---
extern "C" JNIEXPORT jint JNICALL
Java_com_dj_mdumper_NativeBridge_dumpFull(
    JNIEnv* env, jobject,
    jint pid, jstring jpkg, jstring joutDir)
{
    const char* pkg    = env->GetStringUTFChars(jpkg, nullptr);
    const char* outDir = env->GetStringUTFChars(joutDir, nullptr);
    bool bDumpLib = false;
    bool bSuccess = false;
    try {
        bSuccess = RunDump(pid, pkg, EK_MEM_OP_SYSCALL, outDir, bDumpLib, false);
    } catch (const std::exception&) { bSuccess = false; }
    catch (...) { bSuccess = false; }
    env->ReleaseStringUTFChars(jpkg, pkg);
    env->ReleaseStringUTFChars(joutDir, outDir);
    return bSuccess ? 0 : 1;
}

#endif // __ANDROID__
CPP
echo "Appended dumpFull export to $F"
