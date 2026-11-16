package com.dj.mdumper

import android.app.Application

class MDumperApp : Application() {
    companion object {
        init {
            // MobileDumper-7 ships as libMobileDumper-7.so — load it first (has the dumpFull export)
            System.loadLibrary("MobileDumper-7")
            // Our thin wrapper with quick walker + metadata
            System.loadLibrary("mdumper_stub")
        }
    }
}
