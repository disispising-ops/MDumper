package com.dj.mdumper

object NativeBridge {
    /** Quick walker — ESP offsets only, ~1 second. From mdumper_stub.so */
    external fun dump(pid: Int, outDir: String, gameIdentifier: String): Int

    /** FULL SDK dump via MobileDumper-7 — all classes, all offsets, ~1-2 min.
     *  Exported from libMobileDumper-7.so (patched main_android.cpp). */
    external fun dumpFull(pid: Int, packageName: String, outDir: String): Int

    external fun getLastLog(): String
    external fun getLastError(): String
    external fun getBuildInfo(): String
}
