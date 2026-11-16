package com.dj.mdumper

import android.app.Application

class MDumperApp : Application() {
    companion object {
        init {
            System.loadLibrary("mdumper")
        }
    }
}
