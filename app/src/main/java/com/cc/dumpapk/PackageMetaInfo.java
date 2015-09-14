package com.cc.dumpapk;

import android.content.pm.ApplicationInfo;

import de.robv.android.xposed.callbacks.XC_LoadPackage;

/**
 * Created by CwT on 15/7/30.
 */
public class PackageMetaInfo {
    private String packageName;
    private String processName;
    private ClassLoader classLoader;
    private ApplicationInfo appInfo;
    private boolean isFirstApplication;

    public static PackageMetaInfo fromXposed(XC_LoadPackage.LoadPackageParam lpparam){
        PackageMetaInfo pminfo = new PackageMetaInfo();
        pminfo.packageName = lpparam.packageName;
        pminfo.processName = lpparam.processName;
        pminfo.classLoader = lpparam.classLoader;
        pminfo.appInfo = lpparam.appInfo;
        pminfo.isFirstApplication = lpparam.isFirstApplication;
        return pminfo;

    }

    public String getPackageName() {
        return packageName;
    }

    public String getProcessName() {
        return processName;
    }

    public ClassLoader getClassLoader() {
        return classLoader;
    }

    public ApplicationInfo getAppInfo() {
        return appInfo;
    }

    public boolean isFirstApplication() {
        return isFirstApplication;
    }
}
