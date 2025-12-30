# DeskControler_Android

安卓端远程桌面控制客户端

## 功能概述

- 远程桌面视频流接收与显示
- 触摸/鼠标事件远程传输
- QR码扫描快速连接
- WiFi自动连接

---

## Kiosk模式（锁屏模式）

Kiosk模式专为封闭场景设备设计，使平板**完全锁定**于此应用，用户无法进行任何退出或切屏操作。

### 功能特性

1. **开机自动启动** - 设备开机后自动启动应用
2. **完全锁定** - 禁用Home键、返回键、最近任务键
3. **无确认框** - 设置Device Owner后自动锁定，无需用户确认
4. **全屏沉浸式** - 隐藏系统导航栏和状态栏
5. **屏幕常亮** - 防止屏幕自动熄灭
6. **不在最近任务中显示** - 从最近任务列表中隐藏

---

## ⚠️ 重要：设置Device Owner（必需）

要实现**完全锁定、无确认框**的Kiosk模式，**必须**将应用设置为Device Owner。

### 前提条件

设备必须满足以下条件之一：
- **新设备**：未进行过初始设置
- **已恢复出厂设置**：设置 > 系统 > 重置 > 恢复出厂设置
- **无账户**：设置 > 账户，移除所有账户（包括Google账户）

### 设置步骤

#### 1. 安装APK
```bash
adb install DeskControler.apk
```

#### 2. 移除所有账户（重要！）
如果设备已有账户，必须先移除：
```bash
# 查看当前账户
adb shell pm list users

# 或在设备上手动移除：设置 > 账户 > 移除所有账户
```

#### 3. 设置Device Owner
```bash
adb shell dpm set-device-owner org.qtproject.example.DeskControler/.MyDeviceAdminReceiver
```

成功后会显示：
```
Success: Device owner set to package org.qtproject.example.DeskControler
Active admin set to component {org.qtproject.example.DeskControler/org.qtproject.example.DeskControler.MyDeviceAdminReceiver}
```

#### 4. 设置为默认启动器
```bash
# 或在设备上：设置 > 应用 > 默认应用 > 主屏幕应用 > 选择DeskControler
```

#### 5. 重启设备
```bash
adb reboot
```

### 验证Device Owner设置
```bash
adb shell dumpsys device_policy | grep "Device Owner"
```

### 移除Device Owner（调试/恢复用）
```bash
adb shell dpm remove-active-admin org.qtproject.example.DeskControler/.MyDeviceAdminReceiver
```

---

## 调试退出功能

**重要提示：此功能仅供调试阶段使用，正式发布时应删除！**

### 操作方法
在屏幕**左上角100×100像素区域**内，**3秒内连续快速点击5次**即可退出应用。

### 删除调试退出功能（正式发布前）

1. **DeskControler.h** - 删除调试相关常量和成员变量
2. **DeskControler.cpp** - 删除 `mousePressEvent` 中的调试退出代码

---

## 故障排除

### 问题：设置Device Owner失败
**错误信息**：`Not allowed to set the device owner because there are already some accounts on the device`

**解决方案**：
```bash
# 1. 在设备上移除所有账户
# 设置 > 账户 > 点击每个账户 > 移除

# 2. 或恢复出厂设置
# 设置 > 系统 > 重置 > 恢复出厂设置
```

### 问题：应用不是Device Owner，锁定不完全
**现象**：日志显示"应用不是Device Owner，无法完全锁定"

**解决方案**：按上述步骤设置Device Owner

### 问题：开机后应用未自动启动
**解决方案**：
1. 确保将应用设置为默认启动器
2. 部分设备需要在设置中允许"开机自启动"权限

---

## 相关文件

| 文件 | 说明 |
|------|------|
| `AndroidManifest.xml` | 权限、Activity、Receiver配置 |
| `res/xml/device_admin.xml` | Device Admin策略配置 |
| `MyDeviceAdminReceiver.java` | Device Admin接收器 |
| `KioskHelper.java` | Kiosk模式实现 |
| `BootReceiver.java` | 开机广播接收器 |
| `DeskControler.h/cpp` | Qt层Kiosk调用 |

---

## 编译要求

- Qt 5.x with Android support
- Android NDK
- Android SDK (API Level 21+)
- FFmpeg libraries
- QZXing library
- Protocol Buffers

## 许可证

私有项目
