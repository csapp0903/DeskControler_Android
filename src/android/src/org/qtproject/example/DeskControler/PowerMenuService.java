package org.qtproject.example.DeskControler;

import android.accessibilityservice.AccessibilityService;
import android.view.accessibility.AccessibilityEvent;
import android.util.Log;

public class PowerMenuService extends AccessibilityService {
    private static final String TAG = "PowerMenuService";
    private static PowerMenuService instance;

    @Override
    protected void onServiceConnected() {
        super.onServiceConnected();
        Log.i(TAG, "PowerMenuService 已连接");
        instance = this;
        // 将服务实例注册给 KioskHelper
        KioskHelper.setPowerMenuService(this);
    }

    @Override
    public boolean onUnbind(android.content.Intent intent) {
        Log.i(TAG, "PowerMenuService 已断开");
        instance = null;
        KioskHelper.setPowerMenuService(null);
        return super.onUnbind(intent);
    }

    @Override
    public void onAccessibilityEvent(AccessibilityEvent event) {
        // 不需要处理事件
    }

    @Override
    public void onInterrupt() {
        // 服务中断
    }

    // 执行关机菜单动作
    public boolean showPowerMenu() {
        Log.i(TAG, "尝试呼出关机菜单");
        return performGlobalAction(GLOBAL_ACTION_POWER_DIALOG);
    }
}
