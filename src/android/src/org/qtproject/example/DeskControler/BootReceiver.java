package org.qtproject.example.DeskControler;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

/**
 * 开机自动启动接收器
 * Kiosk模式: 设备开机后自动启动应用
 */
public class BootReceiver extends BroadcastReceiver {
    private static final String TAG = "BootReceiver";

    @Override
    public void onReceive(final Context context, Intent intent) {
        String action = intent.getAction();
        Log.i(TAG, "收到广播: " + action);

        if (Intent.ACTION_BOOT_COMPLETED.equals(action) ||
            "android.intent.action.QUICKBOOT_POWERON".equals(action) ||
            "com.htc.intent.action.QUICKBOOT_POWERON".equals(action) ||
            Intent.ACTION_REBOOT.equals(action)) {

            Log.i(TAG, "设备启动完成，准备启动DeskControler应用");

            // 延迟3秒启动，确保系统完全初始化
            new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                @Override
                public void run() {
                    launchApp(context);
                }
            }, 3000);
        }
    }

    private void launchApp(Context context) {
        try {
            // 方法1: 使用包管理器获取启动Intent
            PackageManager pm = context.getPackageManager();
            Intent launchIntent = pm.getLaunchIntentForPackage(context.getPackageName());

            if (launchIntent != null) {
                launchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                launchIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                launchIntent.addFlags(Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
                context.startActivity(launchIntent);
                Log.i(TAG, "应用启动成功 (方法1)");
                return;
            }

            // 方法2: 直接构建Intent
            Intent intent = new Intent();
            intent.setClassName(context.getPackageName(),
                    "org.qtproject.qt5.android.bindings.QtActivity");
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
            context.startActivity(intent);
            Log.i(TAG, "应用启动成功 (方法2)");

        } catch (Exception e) {
            Log.e(TAG, "应用启动失败: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
