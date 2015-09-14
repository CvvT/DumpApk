package com.cc.dumpapk;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import java.util.HashMap;
import java.util.Iterator;

/**
 * Created by CwT on 15/7/31.
 */
public class CommandBroadcastReceiver extends BroadcastReceiver {

    public static String INTENT_ACTION = "com.cc.dumpapk";
    public static String COMMAND_NAME_KEY = "cmd";
    public static String COMMAND_COOKIE = "cookie";

    public static final int DUMP_DEXINFO = 1;
    public static final int DUMP_DEXFILE = 2;

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d("cc", intent.getAction());
        if (INTENT_ACTION.equals(intent.getAction())){
            int cmd = intent.getIntExtra(COMMAND_NAME_KEY, 0);
            int cookie = intent.getIntExtra(COMMAND_COOKIE, 0);
            Log.d("cc", "get cmd:" + cmd + " cookie:" + cookie);
            switch (cmd){
                case DUMP_DEXINFO:
                    dump_dexinfo();
                    break;
                case DUMP_DEXFILE:
                    dump_dexfile(cookie);
                    break;
            }
        }
    }

    private void dump_dexfile(int cookie){
        String filename = ModuleContext.getInstance().getAppContext().getFilesDir() + "/dump.dex";
        ModuleContext.getInstance().dumpDexFile(filename, cookie);
    }

    private void dump_dexinfo(){

        HashMap<String, DexFileInfo> dexfileInfo = ModuleContext.getInstance().dumpDexFileInfo();
        Iterator<DexFileInfo> itor = dexfileInfo.values().iterator();
        DexFileInfo info = null;
        Log.d("cc", "The Dex information");
        while (itor.hasNext()) {
            info = itor.next();
            Log.d("cc", "filepath: "+ info.getDexPath() + ", cookie: " + info.getmCookie());
        }
        Log.d("cc", "End Dex Information");
    }
}
