package com.android.reverse.util;

import android.util.Log;

/**
 * Created by CwT on 15/8/4.
 */
public class NativeFunction {
    private final static String DVMNATIVE_LIB = "dump";
    public static NativeFunction instance;

    static {
        Log.d("cc", "start load library");
        System.loadLibrary(DVMNATIVE_LIB);
        Log.d("cc", "end load library");
    }

    public static NativeFunction getInstance(){
        if (instance == null)
            instance = new NativeFunction();
        return instance;
    }

    public native void saveDexFileByCookie(int cookie, ClassLoader loader);
    public native void dumpDexFileByCookie(int cookie, ClassLoader loader);

}
