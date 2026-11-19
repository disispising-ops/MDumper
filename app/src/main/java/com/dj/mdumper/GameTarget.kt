package com.dj.mdumper

/** A UE4-based game installed on this device. */
data class GameTarget(
    val packageName: String,
    val displayName: String,
    val isRunning: Boolean,
    val pid: Int? = null,
) {
    companion object {
        val PRESETS = listOf(
            "com.tencent.ig"          to "PUBG Mobile (Global)",
            "com.rekoo.pubgm"         to "PUBG Mobile (Indonesia/Taiwan)",
            "com.pubg.imobile"        to "BGMI (India)",
            "com.pubg.krmobile"       to "PUBG Mobile KR/JP",
            "com.vng.pubgmobile"      to "PUBG Mobile (Vietnam)",
            "com.tencent.tmgp.pubgmhd" to "PUBG Mobile HD (CN)",
            // [V-mod] Farlight 84 — Lilith Games UE4 BR
            "com.farlightgames.igame.gp" to "Farlight 84 (Google Play)",
            "com.farlightgames.igame"    to "Farlight 84 (generic)",
            "com.farlightgames.igame.cn" to "Farlight 84 (CN)",
        )
    }
}
