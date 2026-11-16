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
        log("Native: ${NativeBridge.getBuildInfo()}")

        checkStoragePermission()

        b.btnDumpPubg.setOnClickListener {
            dump(GameTarget("com.tencent.ig", "PUBG Mobile (Global)", true))
        }
        b.btnDumpBgmi.setOnClickListener {
            dump(GameTarget("com.pubg.imobile", "BGMI", true))
        }
        b.btnDumpCustom.setOnClickListener {
            pickOtherGame()
        }
        b.btnCheckRoot.setOnClickListener {
            lifecycleScope.launch {
                val ok = withContext(Dispatchers.IO) { RootShell.isRootAvailable() }
                log(if (ok) "✓ Root OK" else "✗ Root denied — grant su to this app")
            }
        }
    }

    private fun checkStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R &&
            !Environment.isExternalStorageManager()) {
            AlertDialog.Builder(this)
                .setTitle("Storage Access Needed")
                .setMessage("MDumper writes dumps to Downloads/MDumper. Grant 'All files access' on the next screen.")
                .setPositiveButton("Grant") { _, _ ->
                    startActivity(Intent(android.provider.Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                        Uri.parse("package:$packageName")))
                }
                .setNegativeButton("Later", null)
                .show()
        }
    }

    private fun pickOtherGame() {
        val choices = GameTarget.PRESETS
        AlertDialog.Builder(this)
            .setTitle("Pick UE4 Game")
            .setItems(choices.map { it.second }.toTypedArray()) { _, i ->
                val (pkg, name) = choices[i]
                dump(GameTarget(pkg, name, true))
            }
            .show()
    }

    private fun dump(target: GameTarget) {
        setBusy(true)
        b.log.text = ""
        log("► Starting dump: ${target.displayName} (${target.packageName})")

        lifecycleScope.launch {
            val result = withContext(Dispatchers.IO) { runDump(target) }
            setBusy(false)
            when {
                result == null                 -> log("✗ Dump failed — check logs above")
                else                           -> onDumpComplete(result, target)
            }
        }
    }

    /** Returns the zip file path on success, null on failure. */
    private fun runDump(target: GameTarget): File? {
        if (!RootShell.isRootAvailable()) {
            log("✗ Root unavailable — grant su and retry")
            return null
        }

        val pids = RootShell.pidsOf(target.packageName)
        if (pids.isEmpty()) {
            log("✗ ${target.packageName} not running — launch it first")
            return null
        }
        val pid = pids.first()
        log("► PID: $pid")

        val stamp = SimpleDateFormat("yyyyMMdd-HHmmss", Locale.US).format(Date())
        val outDir = File(Environment.getExternalStoragePublicDirectory(
            Environment.DIRECTORY_DOWNLOADS), "MDumper/${target.packageName}_$stamp")
        outDir.mkdirs()
        log("► Output: ${outDir.absolutePath}")

        // The actual MobileDumper-7 runs here via JNI
        val rc = try {
            NativeBridge.dump(pid, outDir.absolutePath, target.packageName)
        } catch (t: Throwable) {
            log("✗ Native crash: ${t.message}")
            return null
        }

        // Pull native log regardless of outcome
        log(NativeBridge.getLastLog())
        if (rc != 0) {
            log("✗ Dumper rc=$rc : ${NativeBridge.getLastError()}")
            return null
        }

        log("► Zipping...")
        val zipFile = File(outDir.parentFile, "${outDir.name}.zip")
        zipDir(outDir, zipFile)
        log("✓ Done: ${zipFile.absolutePath} (${zipFile.length() / 1024}KB)")
        return zipFile
    }

    private fun onDumpComplete(zipFile: File, target: GameTarget) {
        AlertDialog.Builder(this)
            .setTitle("Dump Complete")
            .setMessage("${zipFile.name}\n\n${zipFile.length() / 1024}KB written to Downloads/MDumper/")
            .setPositiveButton("Share") { _, _ -> shareFile(zipFile) }
            .setNegativeButton("Close", null)
            .show()
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

// Tiny shim so we don't have to enable buildConfig
object BuildConfigLite {
    const val VERSION_NAME = "0.1.0"
}
