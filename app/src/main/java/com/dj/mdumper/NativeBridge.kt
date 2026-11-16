package com.dj.mdumper

object NativeBridge {
    external fun dump(pid: Int, outDir: String, gameIdentifier: String): Int
    external fun getLastLog(): String
    external fun getLastError(): String
    external fun getBuildInfo(): String
}
