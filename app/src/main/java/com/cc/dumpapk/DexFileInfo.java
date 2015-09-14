package com.cc.dumpapk;

import android.util.Log;

import com.android.reverse.util.NativeFunction;
import com.cc.dumpapk.util.Utility;

import de.robv.android.xposed.XposedBridge;

/**
 * Created by CwT on 15/7/31.
 */
public class DexFileInfo {
    private String dexPath;
    private int mCookie;
    private ClassLoader defineClassLoader;

    public DexFileInfo(String dexPath,int mCookie) {
        super();
        this.dexPath = dexPath;
        this.mCookie = mCookie;
        NativeFunction.getInstance().saveDexFileByCookie(mCookie, ModuleContext.getInstance().getBaseClassLoader());
        XposedBridge.log("dexpath: "+dexPath+", mcookie: "+mCookie);
        Log.d("cc", "dexpath: "+dexPath+", mcookie: "+mCookie);
    }

    public DexFileInfo(String dexPath,int mCookie,ClassLoader classLoader) {
        this(dexPath,mCookie);
        this.defineClassLoader = classLoader;
    }

    public String getDexPath() {
        return dexPath;
    }

    public int getmCookie() {
        return mCookie;
    }

    public void setmCookie(int mCookie) {
        this.mCookie = mCookie;
    }

    public ClassLoader getDefineClassLoader() {
        return defineClassLoader;
    }

    public void setDefineClassLoader(ClassLoader defineClassLoader) {
        this.defineClassLoader = defineClassLoader;
    }

    public void setDexPath(String dexPath) {
        this.dexPath = dexPath;
    }
}
