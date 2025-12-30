package org.qtproject.example.DeskControler;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

/**
 * 开机自动启动接收器
 * Kiosk模式: 设备开机后自动启动应用
 */
public class BootReceiver extends BroadcastReceiver {
    private static final String TAG = "BootReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();

        if (Intent.ACTION_BOOT_COMPLETED.equals(action) ||
            "android.intent.action.QUICKBOOT_POWERON".equals(action)) {

            Log.i(TAG, "设备启动完成，启动DeskControler应用");

            Intent launchIntent = new Intent(context, org.qtproject.qt5.android.bindings.QtActivity.class);
            launchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            launchIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
            launchIntent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);

            try {
                context.startActivity(launchIntent);
                Log.i(TAG, "应用启动成功");
            } catch (Exception e) {
                Log.e(TAG, "应用启动失败: " + e.getMessage());
            }
        }
    }
}
