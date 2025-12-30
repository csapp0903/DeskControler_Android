package org.qtproject.example.DeskControler;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;

/**
 * Kiosk模式辅助类
 * 使用Device Owner实现真正的屏幕锁定（无确认框）
 */
public class KioskHelper {
    private static final String TAG = "KioskHelper";
    private static Handler handler = new Handler(Looper.getMainLooper());
    private static Runnable focusChecker = null;
    private static boolean isKioskEnabled = false;
    private static boolean isDeviceOwner = false;

    /**
     * 启用Kiosk模式
     * @param activity 当前Activity
     */
    public static void enableKioskMode(final Activity activity) {
        if (activity == null) {
            Log.e(TAG, "Activity is null");
            return;
        }

        Log.i(TAG, "启用Kiosk模式");
        isKioskEnabled = true;

        try {
            activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    try {
                        // 1. 保持屏幕常亮
                        activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

                        // 2. 全屏显示
                        activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

                        // 3. 隐藏系统UI
                        hideSystemUI(activity);

                        // 4. 检查是否为Device Owner并启动锁定任务模式
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                            startLockTaskIfDeviceOwner(activity);
                        }

                        // 5. 设置系统UI监听器
                        setupSystemUIListener(activity);

                        // 6. 启动前台监控
                        startFocusMonitor(activity);

                        Log.i(TAG, "Kiosk模式已启用");
                    } catch (Exception e) {
                        Log.e(TAG, "启用Kiosk模式失败: " + e.getMessage());
                    }
                }
            });
        } catch (Exception e) {
            Log.e(TAG, "启用Kiosk模式异常: " + e.getMessage());
        }
    }

    /**
     * 检查是否为Device Owner并启动锁定任务模式
     */
    private static void startLockTaskIfDeviceOwner(Activity activity) {
        try {
            DevicePolicyManager dpm = (DevicePolicyManager) activity.getSystemService(Context.DEVICE_POLICY_SERVICE);
            ComponentName adminComponent = new ComponentName(activity, MyDeviceAdminReceiver.class);

            if (dpm != null && dpm.isDeviceOwnerApp(activity.getPackageName())) {
                isDeviceOwner = true;
                Log.i(TAG, "应用是Device Owner，启用完全锁定模式");

                // 设置锁定任务包白名单
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                    dpm.setLockTaskPackages(adminComponent, new String[]{activity.getPackageName()});
                }

                // 设置为默认启动器（Home）
                setAsDefaultLauncher(activity, dpm, adminComponent);

                // 启动锁定任务模式（Device Owner无需确认）
                activity.startLockTask();
                Log.i(TAG, "锁定任务模式已启动（无确认框）");
            } else {
                isDeviceOwner = false;
                Log.w(TAG, "应用不是Device Owner，无法完全锁定。请通过ADB设置Device Owner。");
                Log.w(TAG, "命令: adb shell dpm set-device-owner org.qtproject.example.DeskControler/.MyDeviceAdminReceiver");
            }
        } catch (Exception e) {
            Log.e(TAG, "启动锁定任务模式失败: " + e.getMessage());
        }
    }

    /**
     * 设置应用为默认启动器（Device Owner专用）
     */
    private static void setAsDefaultLauncher(Activity activity, DevicePolicyManager dpm, ComponentName adminComponent) {
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                // 创建HOME Intent过滤器
                IntentFilter filter = new IntentFilter(android.content.Intent.ACTION_MAIN);
                filter.addCategory(android.content.Intent.CATEGORY_HOME);
                filter.addCategory(android.content.Intent.CATEGORY_DEFAULT);

                // 获取Activity组件名
                ComponentName activityComponent = new ComponentName(
                        activity.getPackageName(),
                        "org.qtproject.qt5.android.bindings.QtActivity"
                );

                // 设置为首选Activity（默认启动器）
                dpm.addPersistentPreferredActivity(adminComponent, filter, activityComponent);
                Log.i(TAG, "已设置为默认启动器");
            }
        } catch (Exception e) {
            Log.e(TAG, "设置默认启动器失败: " + e.getMessage());
        }
    }

    /**
     * 设置系统UI监听器
     */
    private static void setupSystemUIListener(final Activity activity) {
        final View decorView = activity.getWindow().getDecorView();
        decorView.setOnSystemUiVisibilityChangeListener(new View.OnSystemUiVisibilityChangeListener() {
            @Override
            public void onSystemUiVisibilityChange(int visibility) {
                if (isKioskEnabled && (visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) {
                    // 系统UI可见时，立即重新隐藏
                    decorView.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            if (isKioskEnabled) {
                                hideSystemUI(activity);
                            }
                        }
                    }, 100);
                }
            }
        });
    }

    /**
     * 启动前台监控 - 确保应用始终在前台
     */
    private static void startFocusMonitor(final Activity activity) {
        stopFocusMonitor();

        focusChecker = new Runnable() {
            @Override
            public void run() {
                if (!isKioskEnabled) {
                    return;
                }

                try {
                    // 非Device Owner时需要监控前台状态
                    if (!isDeviceOwner && !isAppInForeground(activity)) {
                        Log.i(TAG, "检测到应用不在前台，尝试返回前台");
                        bringToForeground(activity);
                    }

                    // 确保系统UI隐藏
                    hideSystemUI(activity);

                    // 继续监控（Device Owner模式下降低频率）
                    handler.postDelayed(this, isDeviceOwner ? 2000 : 300);
                } catch (Exception e) {
                    Log.e(TAG, "前台监控异常: " + e.getMessage());
                    handler.postDelayed(this, 1000);
                }
            }
        };

        handler.postDelayed(focusChecker, 500);
        Log.i(TAG, "前台监控已启动");
    }

    /**
     * 停止前台监控
     */
    private static void stopFocusMonitor() {
        if (focusChecker != null) {
            handler.removeCallbacks(focusChecker);
            focusChecker = null;
        }
    }

    /**
     * 检查应用是否在前台
     */
    private static boolean isAppInForeground(Activity activity) {
        try {
            return activity.hasWindowFocus();
        } catch (Exception e) {
            return true;
        }
    }

    /**
     * 将应用带到前台
     */
    private static void bringToForeground(Activity activity) {
        try {
            ActivityManager am = (ActivityManager) activity.getSystemService(Context.ACTIVITY_SERVICE);
            if (am != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                am.moveTaskToFront(activity.getTaskId(), ActivityManager.MOVE_TASK_WITH_HOME);
            }
        } catch (Exception e) {
            Log.e(TAG, "返回前台失败: " + e.getMessage());
        }
    }

    /**
     * 隐藏系统UI（导航栏和状态栏）
     */
    public static void hideSystemUI(final Activity activity) {
        if (activity == null) return;

        try {
            activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    View decorView = activity.getWindow().getDecorView();

                    int uiOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

                    decorView.setSystemUiVisibility(uiOptions);
                }
            });
        } catch (Exception e) {
            Log.e(TAG, "隐藏系统UI失败: " + e.getMessage());
        }
    }

    /**
     * 禁用Kiosk模式
     * @param activity 当前Activity
     */
    public static void disableKioskMode(final Activity activity) {
        if (activity == null) {
            return;
        }

        Log.i(TAG, "禁用Kiosk模式");
        isKioskEnabled = false;

        try {
            stopFocusMonitor();

            activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    try {
                        // 停止锁定任务模式
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP && isDeviceOwner) {
                            try {
                                activity.stopLockTask();
                                Log.i(TAG, "锁定任务模式已停止");
                            } catch (Exception e) {
                                Log.w(TAG, "stopLockTask失败: " + e.getMessage());
                            }
                        }

                        activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                        activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
                        showSystemUI(activity);
                        activity.getWindow().getDecorView().setOnSystemUiVisibilityChangeListener(null);

                        Log.i(TAG, "Kiosk模式已禁用");
                    } catch (Exception e) {
                        Log.e(TAG, "禁用Kiosk模式失败: " + e.getMessage());
                    }
                }
            });
        } catch (Exception e) {
            Log.e(TAG, "禁用Kiosk模式异常: " + e.getMessage());
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
     */
    public static void exitApp(final Activity activity) {
        Log.i(TAG, "调试退出: 开始退出流程");

        isKioskEnabled = false;
        stopFocusMonitor();

        if (activity != null) {
            activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    try {
                        // Device Owner模式下需要先清除白名单再停止Lock Task
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                            try {
                                DevicePolicyManager dpm = (DevicePolicyManager) activity.getSystemService(Context.DEVICE_POLICY_SERVICE);
                                ComponentName adminComponent = new ComponentName(activity, MyDeviceAdminReceiver.class);

                                if (dpm != null && dpm.isDeviceOwnerApp(activity.getPackageName())) {
                                    Log.i(TAG, "调试退出: 清除Lock Task白名单");
                                    // 先清除白名单
                                    dpm.setLockTaskPackages(adminComponent, new String[]{});
                                }

                                // 停止Lock Task模式
                                activity.stopLockTask();
                                Log.i(TAG, "调试退出: Lock Task已停止");
                            } catch (Exception e) {
                                Log.e(TAG, "调试退出: stopLockTask失败: " + e.getMessage());
                            }
                        }

                        showSystemUI(activity);

                        // 延迟退出，确保Lock Task完全停止
                        handler.postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                try {
                                    activity.finishAffinity();
                                } catch (Exception e) {
                                    Log.e(TAG, "finishAffinity失败: " + e.getMessage());
                                }

                                handler.postDelayed(new Runnable() {
                                    @Override
                                    public void run() {
                                        Log.i(TAG, "调试退出: 进程退出");
                                        System.exit(0);
                                    }
                                }, 500);
                            }
                        }, 500);
                    } catch (Exception e) {
                        Log.e(TAG, "调试退出异常: " + e.getMessage());
                        System.exit(0);
                    }
                }
            });
        } else {
            System.exit(0);
        }
    }

    /**
     * 检查是否为Device Owner
     */
    public static boolean isDeviceOwner(Context context) {
        try {
            DevicePolicyManager dpm = (DevicePolicyManager) context.getSystemService(Context.DEVICE_POLICY_SERVICE);
            return dpm != null && dpm.isDeviceOwnerApp(context.getPackageName());
        } catch (Exception e) {
            return false;
        }
    }
}
