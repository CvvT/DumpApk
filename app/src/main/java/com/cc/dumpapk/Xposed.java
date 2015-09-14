package com.cc.dumpapk;

import android.content.pm.ApplicationInfo;
import android.util.Log;

import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

/**
 * Created by CwT on 15/7/30.
 */
public class Xposed implements IXposedHookLoadPackage {

    private static final String DUMPAPK = "com.cc.dumpapk";
    private static final String TARGET = "com.cc.test";
    private static final boolean DEBUG = false;
    @Override
    public void handleLoadPackage(XC_LoadPackage.LoadPackageParam loadPackageParam) throws Throwable {
        if (loadPackageParam == null || (loadPackageParam.appInfo.flags &
                (ApplicationInfo.FLAG_SYSTEM | ApplicationInfo.FLAG_UPDATED_SYSTEM_APP)) != 0)
            return;
        Log.d("cc", "loaded app " + loadPackageParam.packageName);
        if (loadPackageParam.isFirstApplication && !DUMPAPK.equals(loadPackageParam.packageName) &&
                TARGET.equals(loadPackageParam.packageName)){
            Log.d("cc", "the package " + loadPackageParam.packageName + " has hooked");
            Log.d("cc", "the target pid is " + android.os.Process.myPid());
            PackageMetaInfo info = PackageMetaInfo.fromXposed(loadPackageParam);
//            ModuleContext.getInstance().initModuleContext(info);
            ModuleContext.getInstance().start(info);
            if (DEBUG){
                Hook.hookdebugmethod();
            }
        }
    }
}
