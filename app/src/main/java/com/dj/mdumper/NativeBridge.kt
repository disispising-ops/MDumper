package com.dj.mdumper

/**
 * JNI into libmdumper.so which wraps MobileDumper-7.
 * All functions are safe to call from any thread; they block until the dumper
 * returns. Call from a background coroutine, never from UI thread.
 */
object NativeBridge {

    /**
     * Dumps a running UE4 process to the given output directory.
     *
     * @param pid  target process PID (must have read access via /proc/<pid>/mem; run under su)
     * @param outDir absolute output path, e.g. /sdcard/Download/MDumper/pubg_v4.7
     * @param gameIdentifier  optional profile name ("com.tencent.ig", etc.) or empty for auto
     * @return 0 on success, non-zero on failure. Log via [getLastError].
     */
    external fun dump(pid: Int, outDir: String, gameIdentifier: String): Int

    /**
     * Returns the last-run log text (stdout+stderr captured from MobileDumper-7).
     */
    external fun getLastLog(): String

    /**
     * Returns a short human-readable error string from the last dump attempt.
     */
    external fun getLastError(): String

    /**
     * Native info: build date, MD7 version, abi.
     */
    external fun getBuildInfo(): String
}
