package com.cc.dumpapk;

import android.app.Application;
import android.content.Context;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.util.Log;

import com.android.reverse.util.NativeFunction;
import com.cc.dumpapk.util.RefInvoke;
import com.cc.dumpapk.util.Utility;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.HashMap;
import java.util.Iterator;

import dalvik.system.DexFile;
import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.XposedHelpers;

/**
 * Created by CwT on 15/7/31.
 */
public class ModuleContext {
    private PackageMetaInfo metaInfo;
    private int apiLevel;
    private boolean HAS_REGISTER_LISENER = false;
    private Application fristApplication;
    private static ModuleContext moduleContext;

    private static HashMap<String, DexFileInfo> dynLoadedDexInfo = new HashMap<String, DexFileInfo>();

    public static ModuleContext getInstance() {
        if (moduleContext == null)
            moduleContext = new ModuleContext();
        return moduleContext;
    }

    public void start(PackageMetaInfo info){
        this.metaInfo = info;
        this.apiLevel = Utility.getApiLevel();

        XposedHelpers.findAndHookMethod("dalvik.system.DexFile", ClassLoader.getSystemClassLoader(),
                "openDexFile", String.class, String.class, int.class, new XC_MethodHook() {
                    @Override
                    protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                        super.afterHookedMethod(param);
                        String dexpath = (String)param.args[0];
                        int mcookie = (Integer)param.getResult();
                        if (mcookie != 0) {
                            dynLoadedDexInfo.put(dexpath, new DexFileInfo(dexpath, mcookie));
                        }
                    }
                });

        XposedHelpers.findAndHookMethod("dalvik.system.DexFile", ClassLoader.getSystemClassLoader(),
                "openDexFile", byte[].class, new XC_MethodHook() {
                    @Override
                    protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                        super.afterHookedMethod(param);
                        int mcookie = (Integer)param.getResult();
                        if (mcookie != 0)
                            dynLoadedDexInfo.put(String.valueOf(mcookie),
                                    new DexFileInfo(String.valueOf(mcookie), mcookie));
                    }
                });

        XposedHelpers.findAndHookMethod("dalvik.system.DexFile", ClassLoader.getSystemClassLoader(),
                "defineClass", String.class, ClassLoader.class, int.class, new XC_MethodHook() {
                    @Override
                    protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                        super.afterHookedMethod(param);
                        if (!param.hasThrowable()){
                            int cookie = (Integer)param.args[2];
                            setDefineClassLoader(cookie, (ClassLoader)param.args[1]);
                        }
                    }
                });
        Log.d("cc", "hook over");
        initModuleContext();
    }

    public void initModuleContext() {
        String appClassName = this.getAppInfo().className;	//Class implementing the Application object,if user didn't define it, the field is null
        Log.d("cc", "application name is : " + appClassName);
        if (appClassName == null){
            XposedHelpers.findAndHookMethod(Application.class.getName(),
                    this.getBaseClassLoader(), "onCreate", new ApplicationOnCreateHook());
        }else{
            Class<?> hook_application_class = null;
            try {
                hook_application_class = this.getBaseClassLoader().loadClass(appClassName);
                if (hook_application_class != null) {
                    //for tecent protect
//                    Method hookOncreateMethod = hook_application_class.getDeclaredMethod("onCreate", new Class[] {});
//                    if (hookOncreateMethod != null)
//                        XposedHelpers.findAndHookMethod(hook_application_class, "onCreate", new ApplicationOnCreateHook());
                    // for ali protect
                    Method hookMethod = hook_application_class.getDeclaredMethod("attachBaseContext", Context.class);
                    if (hookMethod != null)
                        XposedHelpers.findAndHookMethod(hook_application_class, "attachBaseContext", Context.class, new ApplicationOnCreateHook());
                }
            } catch (ClassNotFoundException e) {
                e.printStackTrace();
                return;
            } catch (NoSuchMethodException e) {
                e.printStackTrace();
                XposedHelpers.findAndHookMethod(Application.class.getName(), this.getBaseClassLoader(),
                        "onCreate", new ApplicationOnCreateHook());
                return;
            }
        }
    }
    public String getPackageName() {
        return metaInfo.getPackageName();
    }

    public String getProcssName() {
        return metaInfo.getProcessName();
    }

    public ApplicationInfo getAppInfo() {
        return metaInfo.getAppInfo();
    }

    public Application getAppContext() {
        return this.fristApplication;
    }

    public int getApiLevel() {
        return this.apiLevel;
    }

    public String getLibPath(){
        return this.metaInfo.getAppInfo().nativeLibraryDir;
    }

    public ClassLoader getBaseClassLoader(){
        return this.metaInfo.getClassLoader();
    }

    private class ApplicationOnCreateHook extends XC_MethodHook{

        @Override
        protected void afterHookedMethod(MethodHookParam param) throws Throwable {
            // this will be called after the clock was updated by the original method
            if (!HAS_REGISTER_LISENER) {
                fristApplication = (Application) param.thisObject;
                IntentFilter filter = new IntentFilter(CommandBroadcastReceiver.INTENT_ACTION);
                fristApplication.registerReceiver(new CommandBroadcastReceiver(), filter);
                HAS_REGISTER_LISENER = true;
                Log.d("cc", "register over");
            }
        }
    }

    private void setDefineClassLoader(int mCookie, ClassLoader classLoader){
        Iterator<DexFileInfo> dexinfos = dynLoadedDexInfo.values().iterator();
        DexFileInfo info = null;
        boolean flag = false;
        while(dexinfos.hasNext()){
            info = dexinfos.next();
            if(mCookie == info.getmCookie()){
                if(info.getDefineClassLoader() == null)
                    info.setDefineClassLoader(classLoader);
                flag = true;    //find it
            }
        }
        if (!flag){
            dynLoadedDexInfo.put(String.valueOf(mCookie), new DexFileInfo(String.valueOf(mCookie),
                    mCookie, classLoader));
        }
    }

    public HashMap<String, DexFileInfo> dumpDexFileInfo() {
        HashMap<String, DexFileInfo> dexs = new HashMap<>(dynLoadedDexInfo);
        Object dexPathList = RefInvoke.getFieldOjbect("dalvik.system.BaseDexClassLoader", getBaseClassLoader(), "pathList");
        Object[] dexElements = (Object[]) RefInvoke.getFieldOjbect("dalvik.system.DexPathList", dexPathList, "dexElements");
        DexFile dexFile = null;
        for (int i = 0; i < dexElements.length; i++) {
            dexFile = (DexFile) RefInvoke.getFieldOjbect("dalvik.system.DexPathList$Element", dexElements[i], "dexFile");
            String mFileName = (String) RefInvoke.getFieldOjbect("dalvik.system.DexFile", dexFile, "mFileName");
            int mCookie = RefInvoke.getFieldInt("dalvik.system.DexFile", dexFile, "mCookie");
            DexFileInfo dexinfo = new DexFileInfo(mFileName, mCookie, getBaseClassLoader());
            dexs.put(mFileName, dexinfo);
        }
        return dexs;
    }

    public void dumpDexFile(String filename, int cookie){
        Log.d("cc", "begin dump dex, filename:" + filename + " cookie:" + cookie);
        NativeFunction.getInstance().dumpDexFileByCookie(cookie, getBaseClassLoader());
//        File file = new File(filename);
//        try {
//            if (file.exists() || file.createNewFile()){
//                if (cookie == 0)
//                    return;
//                NativeFunction.dumpDexFileByCookie(cookie, getApiLevel());
//            }
//        } catch (IOException e) {
//            e.printStackTrace();
//            Log.d("cc", "failed");
//        }
        Log.d("cc", "dump dex success");
    }
}
