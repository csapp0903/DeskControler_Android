package org.qtproject.example.DeskControler;

import android.app.Activity;
import android.content.Context;
import android.os.Build;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;

/**
 * Kiosk模式辅助类
 * 提供全屏沉浸式模式和系统UI隐藏功能
 */
public class KioskHelper {
    private static final String TAG = "KioskHelper";

    /**
     * 启用Kiosk模式 - 隐藏系统导航栏和状态栏
     * @param activity 当前Activity
     */
    public static void enableKioskMode(Activity activity) {
        if (activity == null) {
            Log.e(TAG, "Activity is null");
            return;
        }

        Log.i(TAG, "启用Kiosk模式");

        try {
            // 保持屏幕常亮
            activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

            // 全屏显示
            activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

            // 隐藏系统UI
            hideSystemUI(activity);

            // 监听系统UI变化，自动重新隐藏
            final View decorView = activity.getWindow().getDecorView();
            decorView.setOnSystemUiVisibilityChangeListener(new View.OnSystemUiVisibilityChangeListener() {
                @Override
                public void onSystemUiVisibilityChange(int visibility) {
                    if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) {
                        // 系统UI可见时，延迟1秒后重新隐藏
                        decorView.postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                hideSystemUI(activity);
                            }
                        }, 1000);
                    }
                }
            });

            Log.i(TAG, "Kiosk模式已启用");
        } catch (Exception e) {
            Log.e(TAG, "启用Kiosk模式失败: " + e.getMessage());
        }
    }

    /**
     * 隐藏系统UI（导航栏和状态栏）
     */
    public static void hideSystemUI(Activity activity) {
        if (activity == null) return;

        View decorView = activity.getWindow().getDecorView();

        int uiOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

        decorView.setSystemUiVisibility(uiOptions);
    }

    /**
     * 禁用Kiosk模式 - 恢复正常显示
     * @param activity 当前Activity
     */
    public static void disableKioskMode(Activity activity) {
        if (activity == null) {
            Log.e(TAG, "Activity is null");
            return;
        }

        Log.i(TAG, "禁用Kiosk模式");

        try {
            // 移除屏幕常亮
            activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

            // 移除全屏
            activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

            // 恢复系统UI
            showSystemUI(activity);

            // 移除监听器
            activity.getWindow().getDecorView().setOnSystemUiVisibilityChangeListener(null);

            Log.i(TAG, "Kiosk模式已禁用");
        } catch (Exception e) {
            Log.e(TAG, "禁用Kiosk模式失败: " + e.getMessage());
        }
    }

    /**
     * 显示系统UI
     */
    public static void showSystemUI(Activity activity) {
        if (activity == null) return;

        View decorView = activity.getWindow().getDecorView();
        decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
    }

    /**
     * 退出应用（调试用）
     * @param activity 当前Activity
     */
    public static void exitApp(Activity activity) {
        Log.i(TAG, "调试退出: 退出应用");

        if (activity != null) {
            disableKioskMode(activity);
            activity.finishAffinity();
        }

        // 强制退出进程
        System.exit(0);
    }
}
