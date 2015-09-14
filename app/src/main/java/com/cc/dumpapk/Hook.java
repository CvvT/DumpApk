package com.cc.dumpapk;

import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedHelpers;

/**
 * Created by CwT on 15/7/31.
 */
public class Hook {

    public static final String TARGET = "baiduprotect";

    public static void hookdebugmethod(){
        XposedHelpers.findAndHookMethod("android.os.Debug", ModuleContext.getInstance().getBaseClassLoader(),
                "isDebuggerConnected", new XC_MethodHook() {
                    @Override
                    protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                        super.beforeHookedMethod(param);
                        param.setResult(false);
                    }
                });

        XposedHelpers.findAndHookMethod("java.lang.System", ModuleContext.getInstance().getBaseClassLoader(),
                "loadLibrary", String.class, new XC_MethodHook() {
                    @Override
                    protected void afterHookedMethod(MethodHookParam param) throws Throwable {
                        super.beforeHookedMethod(param);
                        String target = (String)param.args[0];
                        Log.d("cc", "load library: " + target);
                        if (TARGET.equals(target)){

//                            android.os.Debug.waitForDebugger();
//                            suspend();
                        }
                    }
                });
    }

    public static void suspend(){
        try {
            int pid = android.os.Process.myPid();
            Log.d("cc", "kill -19 " + pid);
            Process p = Runtime.getRuntime().exec("kill -19 " + pid);
            BufferedReader reader = new BufferedReader(new InputStreamReader(p.getInputStream()));
            BufferedReader error = new BufferedReader(new InputStreamReader(p.getErrorStream()));
            String line = null;
            Log.d("cc", "start output:");
            while ((line = reader.readLine()) != null) {
                Log.d("cc", "output: " + line);
            }
            Log.d("cc", "start error:");
            while ((line = error.readLine()) != null) {
                Log.d("cc", "error :" + line);
            }
            int res = p.waitFor();
            Log.d("cc", "process result is :" + res);
        } catch (InterruptedException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
