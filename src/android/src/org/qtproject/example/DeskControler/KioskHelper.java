package org.qtproject.example.DeskControler;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;

/**
 * Kiosk模式辅助类
 * 提供屏幕锁定和防止用户离开应用的功能
 */
public class KioskHelper {
    private static final String TAG = "KioskHelper";
    private static Handler handler = new Handler(Looper.getMainLooper());
    private static Runnable focusChecker = null;
    private static boolean isKioskEnabled = false;

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
            // 在UI线程执行
            activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    try {
                        // 1. 保持屏幕常亮
                        activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

                        // 2. 全屏显示
                        activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

                        // 3. 防止截屏（可选，增加安全性）
                        // activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_SECURE);

                        // 4. 隐藏系统UI
                        hideSystemUI(activity);

                        // 5. 启动屏幕锁定任务模式 (Android 5.0+)
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                            try {
                                activity.startLockTask();
                                Log.i(TAG, "屏幕锁定任务模式已启动");
                            } catch (Exception e) {
                                Log.w(TAG, "startLockTask失败(需要用户确认或Device Owner权限): " + e.getMessage());
                            }
                        }

                        // 6. 监听系统UI变化，自动重新隐藏
                        setupSystemUIListener(activity);

                        // 7. 启动前台监控，确保应用始终在前台
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
                    }, 500);
                }
            }
        });
    }

    /**
     * 启动前台监控 - 确保应用始终在前台
     */
    private static void startFocusMonitor(final Activity activity) {
        // 停止之前的监控
        stopFocusMonitor();

        focusChecker = new Runnable() {
            @Override
            public void run() {
                if (!isKioskEnabled) {
                    return;
                }

                try {
                    // 检查应用是否在前台
                    if (!isAppInForeground(activity)) {
                        Log.i(TAG, "检测到应用不在前台，尝试返回前台");
                        bringToForeground(activity);
                    }

                    // 确保系统UI隐藏
                    hideSystemUI(activity);

                    // 继续监控
                    handler.postDelayed(this, 500);
                } catch (Exception e) {
                    Log.e(TAG, "前台监控异常: " + e.getMessage());
                    handler.postDelayed(this, 1000);
                }
            }
        };

        handler.postDelayed(focusChecker, 1000);
        Log.i(TAG, "前台监控已启动");
    }

    /**
     * 停止前台监控
     */
    private static void stopFocusMonitor() {
        if (focusChecker != null) {
            handler.removeCallbacks(focusChecker);
            focusChecker = null;
            Log.i(TAG, "前台监控已停止");
        }
    }

    /**
     * 检查应用是否在前台
     */
    private static boolean isAppInForeground(Activity activity) {
        try {
            ActivityManager am = (ActivityManager) activity.getSystemService(Context.ACTIVITY_SERVICE);
            if (am != null) {
                // 检查应用是否在运行任务的顶部
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    return activity.hasWindowFocus();
                } else {
                    java.util.List<ActivityManager.RunningTaskInfo> tasks = am.getRunningTasks(1);
                    if (tasks != null && !tasks.isEmpty()) {
                        String topActivity = tasks.get(0).topActivity.getPackageName();
                        return topActivity.equals(activity.getPackageName());
                    }
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "检查前台状态失败: " + e.getMessage());
        }
        return true; // 默认认为在前台
    }

    /**
     * 将应用带到前台
     */
    private static void bringToForeground(Activity activity) {
        try {
            ActivityManager am = (ActivityManager) activity.getSystemService(Context.ACTIVITY_SERVICE);
            if (am != null) {
                // 方法1: moveTaskToFront
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                    am.moveTaskToFront(activity.getTaskId(), ActivityManager.MOVE_TASK_WITH_HOME);
                }
            }

            // 方法2: 重新激活Activity
            // Intent intent = new Intent(activity, activity.getClass());
            // intent.addFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
            // intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
            // activity.startActivity(intent);

            Log.i(TAG, "应用已返回前台");
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
     * 禁用Kiosk模式 - 恢复正常显示
     * @param activity 当前Activity
     */
    public static void disableKioskMode(final Activity activity) {
        if (activity == null) {
            Log.e(TAG, "Activity is null");
            return;
        }

        Log.i(TAG, "禁用Kiosk模式");
        isKioskEnabled = false;

        try {
            // 停止前台监控
            stopFocusMonitor();

            activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    try {
                        // 停止屏幕锁定任务模式
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                            try {
                                activity.stopLockTask();
                                Log.i(TAG, "屏幕锁定任务模式已停止");
                            } catch (Exception e) {
                                Log.w(TAG, "stopLockTask失败: " + e.getMessage());
                            }
                        }

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
     * @param activity 当前Activity
     */
    public static void exitApp(final Activity activity) {
        Log.i(TAG, "调试退出: 退出应用");

        // 先禁用Kiosk模式
        isKioskEnabled = false;
        stopFocusMonitor();

        if (activity != null) {
            activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    try {
                        // 停止锁定任务
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                            try {
                                activity.stopLockTask();
                            } catch (Exception e) {
                                Log.w(TAG, "stopLockTask失败: " + e.getMessage());
                            }
                        }

                        // 恢复系统UI
                        showSystemUI(activity);

                        // 结束Activity
                        activity.finishAffinity();

                        // 延迟退出进程
                        handler.postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                System.exit(0);
                            }
                        }, 500);
                    } catch (Exception e) {
                        Log.e(TAG, "退出应用失败: " + e.getMessage());
                        System.exit(0);
                    }
                }
            });
        } else {
            System.exit(0);
        }
    }
}
