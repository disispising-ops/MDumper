package com.dj.mdumper

/**
 * STUB build — native dumper integration pending.
 * Real dump work still happens via Termux + MobileDumper-7 for now.
 */
object NativeBridge {
    fun dump(pid: Int, outDir: String, gameIdentifier: String): Int {
        return -1
    }
    fun getLastLog(): String = "Stub build: native dumper not linked yet."
    fun getLastError(): String = "native dumper disabled in this build"
    fun getBuildInfo(): String = "Stub, arm64-v8a"
}
