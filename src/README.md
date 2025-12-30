# DeskControler_Android

安卓端远程桌面控制客户端

## 功能概述

- 远程桌面视频流接收与显示
- 触摸/鼠标事件远程传输
- QR码扫描快速连接
- WiFi自动连接

---

## Kiosk模式（锁屏模式）

Kiosk模式是专为封闭场景设备设计的功能，使平板专用于运行此应用，用户无法退出应用进入系统桌面。

### 功能特性

1. **开机自动启动** - 设备开机后自动启动应用
2. **设置为默认启动器** - 按Home键不会退出应用
3. **全屏沉浸式模式** - 隐藏系统导航栏和状态栏
4. **屏幕常亮** - 防止屏幕自动熄灭
5. **返回键拦截** - 禁用系统返回键
6. **系统UI自动隐藏** - 如果用户尝试呼出系统UI，会自动重新隐藏

### 调试退出功能

**重要提示：此功能仅供调试阶段使用，正式发布时应删除！**

在Kiosk模式下，可以通过以下方式强制退出应用：

- **操作方法**：在屏幕**左上角100x100像素区域**内，**3秒内连续快速点击5次**
- 每次点击会在日志中显示当前计数
- 达到5次后应用会自动退出

### 启用/禁用Kiosk模式

#### 启用Kiosk模式（默认）

Kiosk模式默认启用。代码位置：`DeskControler.cpp` 构造函数

```cpp
// ============ 启用Kiosk模式 ============
#ifdef Q_OS_ANDROID
    enableKioskMode();
#endif
```

#### 禁用Kiosk模式

如需禁用Kiosk模式，注释掉上述代码即可：

```cpp
// ============ 启用Kiosk模式 ============
#ifdef Q_OS_ANDROID
    // enableKioskMode();  // 注释此行以禁用Kiosk模式
#endif
```

### 设置为默认启动器

首次安装应用后，需要在系统设置中将应用设置为默认启动器：

1. 打开系统**设置 > 应用 > 默认应用 > 主屏幕应用**
2. 选择 **DeskControler**
3. 之后按Home键将返回此应用而非系统桌面

### 删除调试退出功能

正式发布前，请删除以下代码：

1. **DeskControler.h** - 删除调试退出相关常量：
   ```cpp
   // 删除以下行
   static const int DEBUG_EXIT_TAP_COUNT = 5;
   static const int DEBUG_EXIT_TAP_TIMEOUT_MS = 3000;
   static const int DEBUG_EXIT_TAP_AREA_SIZE = 100;
   ```

2. **DeskControler.h** - 删除调试退出相关成员变量：
   ```cpp
   // 删除以下行
   int m_debugExitTapCount = 0;
   QElapsedTimer m_debugExitTimer;
   ```

3. **DeskControler.cpp** - 删除 `mousePressEvent` 函数中调试退出相关的代码块

4. **KioskHelper.java** - 删除 `exitApp` 方法（可选）

### 相关文件

| 文件 | 说明 |
|------|------|
| `AndroidManifest.xml` | 包含开机广播权限和HOME启动器配置 |
| `BootReceiver.java` | 处理开机广播，自动启动应用 |
| `KioskHelper.java` | Android层Kiosk模式辅助类 |
| `DeskControler.h/cpp` | Qt层Kiosk模式实现 |

---

## 编译要求

- Qt 5.x with Android support
- Android NDK
- FFmpeg libraries
- QZXing library
- Protocol Buffers

## 许可证

私有项目
