package com.huajiqaq.sgamehook

import de.robv.android.xposed.IXposedHookLoadPackage
import de.robv.android.xposed.callbacks.XC_LoadPackage
import android.util.Log

class XposedMod : IXposedHookLoadPackage {
    private val TAG = "SGAMEHOOK"

    override fun handleLoadPackage(lpparam: XC_LoadPackage.LoadPackageParam) {
        val packageName = lpparam.packageName
        val processName = lpparam.processName

        if ((packageName == "com.tencent.tmgp.sgame" || packageName == "com.tencent.tmgp.sgamece") && packageName == processName) {
            Log.d(TAG, "Package detected: $packageName, Process: $processName")
            System.loadLibrary("sgame_hook");
        }
    }

}