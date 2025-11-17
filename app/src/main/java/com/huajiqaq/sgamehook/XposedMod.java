package com.huajiqaq.sgamehook;

import android.util.Log;
import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

public class XposedMod implements IXposedHookLoadPackage {
    private static final String TAG = "XPOSED_SGAMEHOOK";

    @Override
    public void handleLoadPackage(XC_LoadPackage.LoadPackageParam lpparam) {
        String packageName = lpparam.packageName;
        String processName = lpparam.processName;

        if ((packageName.equals("com.tencent.tmgp.sgame") || packageName.equals("com.tencent.tmgp.sgamece")) && packageName.equals(processName)) {
            Log.d(TAG, "Package detected: " + packageName + ", Process: " + processName);
            System.loadLibrary("sgame_hook");
        }
    }
}