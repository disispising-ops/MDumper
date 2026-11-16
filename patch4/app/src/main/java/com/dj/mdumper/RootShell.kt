package com.dj.mdumper

import java.io.BufferedReader
import java.io.DataOutputStream
import java.io.InputStreamReader

/**
 * Minimal root-shell helper. Spawns `su`, feeds commands via stdin, captures stdout+stderr.
 * No libsu dependency to keep APK tiny. KernelSU / Magisk / SuperSU all compatible.
 */
object RootShell {

    data class Result(val exitCode: Int, val stdout: String, val stderr: String)

    fun isRootAvailable(): Boolean = try {
        exec("id").exitCode == 0
    } catch (_: Throwable) { false }

    /** Runs [cmd] via `sh -c`. */
    fun exec(cmd: String, timeoutMs: Long = 30_000L): Result {
        val proc = Runtime.getRuntime().exec(arrayOf("su", "-c", cmd))
        val out = StringBuilder()
        val err = StringBuilder()

        val outThread = Thread { proc.inputStream.bufferedReader().forEachLine { out.appendLine(it) } }
        val errThread = Thread { proc.errorStream.bufferedReader().forEachLine { err.appendLine(it) } }
        outThread.start(); errThread.start()

        val finished = proc.waitFor(timeoutMs, java.util.concurrent.TimeUnit.MILLISECONDS)
        if (!finished) {
            proc.destroyForcibly()
            return Result(-1, out.toString(), err.toString() + "\n[timeout after ${timeoutMs}ms]")
        }
        outThread.join(500); errThread.join(500)
        return Result(proc.exitValue(), out.toString(), err.toString())
    }

    /** Gets PIDs matching an exact process name (cmdline). */
    fun pidsOf(cmdline: String): List<Int> {
        val r = exec("pgrep -f \"^$cmdline\\$\"")
        return r.stdout.lines().mapNotNull { it.trim().toIntOrNull() }
    }

    /** Returns true if /data/app/... contains libUE4.so for the given package. */
    fun isUE4Package(pkg: String): Boolean {
        val r = exec("find /data/app -name libUE4.so -path '*$pkg*' 2>/dev/null | head -1")
        return r.stdout.isNotBlank()
    }
}
