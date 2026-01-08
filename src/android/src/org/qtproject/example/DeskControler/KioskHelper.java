package org.qtproject.example.DeskControler;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

/**
 * Kiosk模式辅助类
 * 使用Device Owner实现真正的屏幕锁定（无确认框）
 *
 * 修复：底部黑色区域问题 - 通过设置透明导航栏和正确的布局标志
 */
public class KioskHelper {
    private static final String TAG = "KioskHelper";
    private static Handler handler = new Handler(Looper.getMainLooper());
    private static Runnable focusChecker = null;
    private static boolean isKioskEnabled = false;
    private static boolean isDeviceOwner = false;
    private static PowerMenuService powerMenuService = null;

    /**
     * 设置PowerMenuService实例
     * @param service PowerMenuService实例，或null表示服务断开
     */
    public static void setPowerMenuService(PowerMenuService service) {
        powerMenuService = service;
        if (service != null) {
            Log.i(TAG, "PowerMenuService 已注册");
        } else {
            Log.i(TAG, "PowerMenuService 已注销");
        }
    }

    /**
     * 获取PowerMenuService实例
     * @return PowerMenuService实例，如果服务未连接则返回null
     */
    public static PowerMenuService getPowerMenuService() {
        return powerMenuService;
    }

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

        // 确保Kiosk组件已启用
        enableKioskComponents(activity);

        try {
            activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    try {
                        Window window = activity.getWindow();

                        // 1. 设置窗口标志 - 全屏、常亮
                        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

                        // 2. 关键修复：设置布局延伸到系统栏区域，避免黑色区域
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                            // 清除旧的全屏标志，使用新的沉浸式方式
                            window.clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

                            // 允许绘制系统栏背景，并设置为透明
                            window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
                            window.setStatusBarColor(Color.TRANSPARENT);
                            window.setNavigationBarColor(Color.TRANSPARENT);

                            // 设置DecorView布局延伸到系统栏下方
                            View decorView = window.getDecorView();
                            decorView.setSystemUiVisibility(
                                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            );
                        } else {
                            window.addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
                        }

                        // 3. 针对刘海屏的处理 (Android P及以上)
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                            WindowManager.LayoutParams lp = window.getAttributes();
                            lp.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
                            window.setAttributes(lp);
                        }

                        // 4. 隐藏系统UI
                        hideSystemUI(activity);

                        // 5. 检查是否为Device Owner并启动锁定任务模式
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                            startLockTaskIfDeviceOwner(activity);
                        }

                        // 6. 设置系统UI监听器
                        setupSystemUIListener(activity);

                        // 7. 启动前台监控
                        startFocusMonitor(activity);

                        // 8. 多次延迟隐藏系统UI，确保完全生效
                        // 首次延迟100ms
                        handler.postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                if (isKioskEnabled) {
                                    hideSystemUI(activity);
                                }
                            }
                        }, 100);

                        // 二次延迟500ms
                        handler.postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                if (isKioskEnabled) {
                                    hideSystemUI(activity);
                                }
                            }
                        }, 500);

                        // 三次延迟1000ms（应对某些慢速设备）
                        handler.postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                if (isKioskEnabled) {
                                    hideSystemUI(activity);
                                }
                            }
                        }, 1000);

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
                if (isKioskEnabled) {
                    // 检测系统UI是否变得可见
                    boolean systemBarsVisible = (visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0;
                    if (systemBarsVisible) {
                        // 系统UI可见时，立即重新隐藏
                        decorView.postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                if (isKioskEnabled) {
                                    hideSystemUI(activity);
                                }
                            }
                        }, 50); // 缩短延迟时间，更快响应
                    }
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
     * 修复：使用正确的标志组合，避免底部黑色区域
     */
    public static void hideSystemUI(final Activity activity) {
        if (activity == null) return;

        try {
            activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    Window window = activity.getWindow();
                    View decorView = window.getDecorView();

                    // Android 11 (API 30) 及以上使用新的 WindowInsetsController
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                        WindowInsetsController controller = window.getInsetsController();
                        if (controller != null) {
                            controller.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
                            controller.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                        }
                    } else {
                        // Android 10及以下使用传统方式
                        int uiOptions = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                                | View.SYSTEM_UI_FLAG_FULLSCREEN
                                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

                        // 低光模式 - 使导航栏图标变暗（某些设备上有效）
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
                            uiOptions |= View.SYSTEM_UI_FLAG_LOW_PROFILE;
                        }

                        decorView.setSystemUiVisibility(uiOptions);
                    }

                    // 确保导航栏颜色透明
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                        window.setNavigationBarColor(Color.TRANSPARENT);
                        window.setStatusBarColor(Color.TRANSPARENT);
                    }
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

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            WindowInsetsController controller = activity.getWindow().getInsetsController();
            if (controller != null) {
                controller.show(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
            }
        } else {
            decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        }
    }


    /**
     * 显示电源菜单（关机/重启）
     */
    public static void showPowerMenu(final Activity activity) {
        if (activity == null) return;

        try {
            // 优先使用AccessibilityService方式呼出电源菜单
            if (powerMenuService != null) {
                boolean success = powerMenuService.showPowerMenu();
                if (success) {
                    Log.i(TAG, "通过AccessibilityService呼出电源菜单成功");
                    return;
                }
            }

            // 备选方案：尝试使用Device Policy Manager的关机功能（需要Device Owner）
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                DevicePolicyManager dpm = (DevicePolicyManager) activity.getSystemService(Context.DEVICE_POLICY_SERVICE);
                if (dpm != null && dpm.isDeviceOwnerApp(activity.getPackageName())) {
                    ComponentName adminComponent = new ComponentName(activity, MyDeviceAdminReceiver.class);
                    // 重启设备
                    // dpm.reboot(adminComponent); // 取消注释以启用
                    Log.i(TAG, "Device Owner可以使用reboot功能");
                }
            }

            Log.w(TAG, "无法呼出电源菜单，PowerMenuService未连接且非Device Owner");
        } catch (Exception e) {
            Log.e(TAG, "显示电源菜单失败: " + e.getMessage());
        }
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
                        DevicePolicyManager dpm = (DevicePolicyManager) activity.getSystemService(Context.DEVICE_POLICY_SERVICE);
                        ComponentName adminComponent = new ComponentName(activity, MyDeviceAdminReceiver.class);
                        PackageManager pm = activity.getPackageManager();

                        // 1. 禁用BootReceiver，防止开机自启动
                        Log.i(TAG, "调试退出: 禁用BootReceiver");
                        try {
                            ComponentName bootReceiver = new ComponentName(activity, BootReceiver.class);
                            pm.setComponentEnabledSetting(bootReceiver,
                                    PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                                    PackageManager.DONT_KILL_APP);
                        } catch (Exception e) {
                            Log.e(TAG, "禁用BootReceiver失败: " + e.getMessage());
                        }

                        if (dpm != null && dpm.isDeviceOwnerApp(activity.getPackageName())) {
                            // 2. 清除默认启动器设置 (解除 Device Owner 的强制 Home 设置)
                            Log.i(TAG, "调试退出: 清除默认启动器设置");
                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                                dpm.clearPackagePersistentPreferredActivities(adminComponent, activity.getPackageName());
                            }

                            // 3. 清除Lock Task白名单
                            Log.i(TAG, "调试退出: 清除Lock Task白名单");
                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                                dpm.setLockTaskPackages(adminComponent, new String[]{});
                            }
                        }

                        // 4. 停止Lock Task模式
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                            try {
                                activity.stopLockTask();
                                Log.i(TAG, "调试退出: Lock Task已停止");
                            } catch (Exception e) {
                                Log.e(TAG, "调试退出: stopLockTask失败: " + e.getMessage());
                            }
                        }

                        showSystemUI(activity);

                        // ==========================================
                        // [关键修复] 跳转到系统设置，打断 Home 重启循环
                        // ==========================================
                        try {
                            // 尝试打开"主屏幕应用"设置，让用户选择原生桌面
                            Intent intent = new Intent(android.provider.Settings.ACTION_HOME_SETTINGS);
                            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                            if (intent.resolveActivity(activity.getPackageManager()) != null) {
                                activity.startActivity(intent);
                            } else {
                                // 如果没有Home设置页，则打开通用设置页
                                Intent settingsIntent = new Intent(android.provider.Settings.ACTION_SETTINGS);
                                settingsIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                                activity.startActivity(settingsIntent);
                            }
                            Log.i(TAG, "调试退出: 已跳转至系统设置");
                        } catch (Exception e) {
                            Log.e(TAG, "跳转系统设置失败: " + e.getMessage());
                        }

                        // 5. 延迟退出，确保设置页面已经覆盖上来
                        handler.postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                Log.i(TAG, "调试退出: 结束Activity");
                                try {
                                    // 移除从最近任务列表
                                    activity.finishAndRemoveTask();
                                } catch (Exception e) {
                                    try {
                                        activity.finishAffinity();
                                    } catch (Exception ex) {
                                        ex.printStackTrace();
                                    }
                                }

                                handler.postDelayed(new Runnable() {
                                    @Override
                                    public void run() {
                                        Log.i(TAG, "调试退出: 终止进程");
                                        android.os.Process.killProcess(android.os.Process.myPid());
                                        System.exit(0);
                                    }
                                }, 500); // 稍微增加延迟，给系统处理 Intent 的时间
                            }
                        }, 1000); // 增加第一段延迟
                    } catch (Exception e) {
                        Log.e(TAG, "调试退出异常: " + e.getMessage());
                        android.os.Process.killProcess(android.os.Process.myPid());
                    }
                }
            });
        } else {
            android.os.Process.killProcess(android.os.Process.myPid());
        }
    }

    /**
     * 重新启用Kiosk组件（应用启动时调用）
     */
    public static void enableKioskComponents(Context context) {
        try {
            PackageManager pm = context.getPackageManager();
            ComponentName bootReceiver = new ComponentName(context, BootReceiver.class);
            pm.setComponentEnabledSetting(bootReceiver,
                    PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                    PackageManager.DONT_KILL_APP);
            Log.i(TAG, "BootReceiver已启用");
        } catch (Exception e) {
            Log.e(TAG, "启用BootReceiver失败: " + e.getMessage());
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
