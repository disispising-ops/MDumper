package com.dj.mdumper

import android.app.Application

class MDumperApp : Application() {
    companion object {
        init {
            // Loads libmdumper.so built from CMake (wraps MobileDumper-7)
            System.loadLibrary("mdumper")
        }
    }
}
