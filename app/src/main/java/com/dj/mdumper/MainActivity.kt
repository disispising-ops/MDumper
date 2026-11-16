package com.dj.mdumper

import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.FileProvider
import androidx.lifecycle.lifecycleScope
import com.dj.mdumper.databinding.ActivityMainBinding
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream

class MainActivity : AppCompatActivity() {

    private lateinit var b: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        b = ActivityMainBinding.inflate(layoutInflater)
        setContentView(b.root)

        log("MDumper ${BuildConfigLite.VERSION_NAME}")
        try { log("Native: ${NativeBridge.getBuildInfo()}") }
        catch (t: Throwable) { log("Native lib not loaded: ${t.message}") }

        log("Helper (quick): ${quickHelper().absolutePath}")
        log("Helper (full):  ${fullHelper().absolutePath}")
        log("Quick exists:   ${quickHelper().exists()}")
        log("Full exists:    ${fullHelper().exists()}")

        checkStoragePermission()

        b.btnDumpPubg.setOnClickListener {
            dump(GameTarget("com.tencent.ig", "PUBG Mobile (Global)", true))
        }
        b.btnDumpBgmi.setOnClickListener {
            dump(GameTarget("com.pubg.imobile", "BGMI", true))
        }
        b.btnDumpCustom.setOnClickListener { pickOtherGame() }
        b.btnCheckRoot.setOnClickListener {
            lifecycleScope.launch {
                val ok = withContext(Dispatchers.IO) { RootShell.isRootAvailable() }
                log(if (ok) "✓ Root OK" else "✗ Root denied — grant su")
            }
        }
    }

    private fun quickHelper(): File = File(applicationInfo.nativeLibraryDir, "libmdumper_exec.so")
    private fun fullHelper():  File = File(applicationInfo.nativeLibraryDir, "libmdumper_full.so")

    private fun checkStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R &&
            !Environment.isExternalStorageManager()) {
            AlertDialog.Builder(this)
                .setTitle("Storage Access Needed")
                .setMessage("MDumper writes dumps to Downloads/MDumper.")
                .setPositiveButton("Grant") { _, _ ->
                    startActivity(Intent(android.provider.Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                        Uri.parse("package:$packageName")))
                }.setNegativeButton("Later", null).show()
        }
    }

    private fun pickOtherGame() {
        val c = GameTarget.PRESETS
        AlertDialog.Builder(this).setTitle("Pick UE4 Game")
            .setItems(c.map { it.second }.toTypedArray()) { _, i ->
                dump(GameTarget(c[i].first, c[i].second, true))
            }.show()
    }

    private fun dump(target: GameTarget) {
        setBusy(true)
        b.log.text = ""
        log("► Starting dump: ${target.displayName} (${target.packageName})")
        lifecycleScope.launch {
            val result = withContext(Dispatchers.IO) { runDump(target) }
            setBusy(false)
            if (result == null) log("✗ Dump failed — check logs above")
            else onDumpComplete(result)
        }
    }

    private fun runDump(target: GameTarget): File? {
        if (!RootShell.isRootAvailable()) { log("✗ Root unavailable"); return null }
        val pids = RootShell.pidsOf(target.packageName)
        if (pids.isEmpty()) { log("✗ ${target.packageName} not running"); return null }
        val pid = pids.first()
        log("► PID: $pid")

        val stamp = SimpleDateFormat("yyyyMMdd-HHmmss", Locale.US).format(Date())
        val outDir = File(Environment.getExternalStoragePublicDirectory(
            Environment.DIRECTORY_DOWNLOADS), "MDumper/${target.packageName}_$stamp")
        outDir.mkdirs()
        log("► Output: ${outDir.absolutePath}")

        // ─── PATH 1: run MD7 as a root subprocess via `su -c libmdumper_full.so` ──
        val full = fullHelper()
        if (full.exists()) {
            log("► Running full MD7 dumper via su (may take 1-2 min)...")
            // MD7 executable accepts args; see its DUMPER_BUILD_EXECUTABLE main.
            // Usage pattern: mdumper_full -p <pid> -o <outdir>  OR  mdumper_full <pid> <outdir>
            // We try the common forms.
            val cmd = "${full.absolutePath} -p ${target.packageName} -o '${outDir.absolutePath}' -m 1"
            log("$ $cmd")
            val r = RootShell.exec(cmd, timeoutMs = 300_000) // 5 min
            if (r.stdout.isNotBlank()) log(r.stdout)
            if (r.stderr.isNotBlank()) log("[stderr] ${r.stderr}")
            log("► exit=${r.exitCode}")
            // Did anything land in outDir?
            val files = outDir.walkTopDown().filter { it.isFile }.toList()
            log("► Files written: ${files.size}")
            if (files.isNotEmpty()) {
                val zipFile = File(outDir.parentFile, "${outDir.name}.zip")
                zipDir(outDir, zipFile)
                log("✓ Done: ${zipFile.length() / 1024}KB")
                return zipFile
            }
            log("✗ Full dumper produced no output — falling back to quick walker")
        } else {
            log("✗ libmdumper_full.so missing — only quick walker available")
        }

        // ─── PATH 2: quick walker fallback ───────────────────────────────────────
        val quick = quickHelper()
        if (!quick.exists()) { log("✗ No helper binaries"); return null }
        log("► Running quick walker via su...")
        val r = RootShell.exec("${quick.absolutePath} '${outDir.absolutePath}'", 60_000)
        log(r.stdout)
        if (r.stderr.isNotBlank()) log("[stderr] ${r.stderr}")
        val files = outDir.walkTopDown().filter { it.isFile }.toList()
        if (files.isEmpty()) return null
        val zipFile = File(outDir.parentFile, "${outDir.name}.zip")
        zipDir(outDir, zipFile)
        log("✓ Done: ${zipFile.length() / 1024}KB")
        return zipFile
    }

    private fun onDumpComplete(zipFile: File) {
        AlertDialog.Builder(this)
            .setTitle("Dump Complete")
            .setMessage("${zipFile.name}\n${zipFile.length() / 1024}KB")
            .setPositiveButton("Share") { _, _ -> shareFile(zipFile) }
            .setNegativeButton("Close", null).show()
    }

    private fun shareFile(f: File) {
        val uri = FileProvider.getUriForFile(this, "$packageName.fileprovider", f)
        val share = Intent(Intent.ACTION_SEND).apply {
            type = "application/zip"
            putExtra(Intent.EXTRA_STREAM, uri)
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        }
        startActivity(Intent.createChooser(share, "Share dump"))
    }

    private fun setBusy(busy: Boolean) {
        b.progress.visibility = if (busy) android.view.View.VISIBLE else android.view.View.GONE
        b.btnDumpPubg.isEnabled = !busy
        b.btnDumpBgmi.isEnabled = !busy
        b.btnDumpCustom.isEnabled = !busy
    }

    private fun log(s: String) {
        runOnUiThread {
            b.log.append(s)
            if (!s.endsWith("\n")) b.log.append("\n")
            b.logScroll.post { b.logScroll.fullScroll(android.view.View.FOCUS_DOWN) }
        }
    }

    private fun zipDir(src: File, dst: File) {
        ZipOutputStream(dst.outputStream().buffered()).use { zos ->
            src.walkTopDown().filter { it.isFile }.forEach { f ->
                val rel = f.relativeTo(src.parentFile!!).path
                zos.putNextEntry(ZipEntry(rel))
                f.inputStream().copyTo(zos)
                zos.closeEntry()
            }
        }
    }
}

object BuildConfigLite { const val VERSION_NAME = "0.4.5" }
