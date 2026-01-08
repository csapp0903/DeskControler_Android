#pragma once

#include <QWidget>
#include <QCamera>
#include <QScrollArea>
#include <QElapsedTimer>
#include <QStackedLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QtConcurrent>
#include <QFuture>
#include <QPainter>
#include <QPainterPath>

#include "NetworkManager.h"
#include "VideoReceiver.h"
#include "ui_DeskControler.h"
// #include "RemoteClipboard.h"

#include "AndroidVideoSurface.h"
#include "QZXing.h"

class NetworkManager;

class CameraPreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit CameraPreviewWidget(QWidget *parent = nullptr) : QWidget(parent) {
        // 不擦除背景，直接绘制
        setAttribute(Qt::WA_OpaquePaintEvent);
    }

    void setImage(const QImage &img) {
        m_frame = img;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        if (m_frame.isNull()) {
            p.fillRect(rect(), Qt::black);
            return;
        }

        // 绘制视频帧 (自动缩放)
        p.drawImage(rect(), m_frame);

        // // 绘制扫描框 (直接在控件上绘制，无需修改图像数据)
        // drawScanBox(p);

        // === 绘制半透明遮罩 (挖空中间) ===
        int w = width();
        int h = height();
        int boxSize = qMin(w, h) * 0.6;
        int x = (w - boxSize) / 2;
        int y = (h - boxSize) / 2;
        QRect scanRect(x, y, boxSize, boxSize);

        // 使用奇偶填充规则来实现挖空效果
        QPainterPath path;
        path.addRect(rect());       // 添加整个窗口区域
        path.addRect(scanRect);     // 添加中间扫描框区域

        p.setBrush(QColor(0, 0, 0, 150)); // 半透明黑色 (Alpha 150/255), 150 是 Alpha 通道（透明度），100 会更亮，200 会更暗
        p.setPen(Qt::NoPen);

        // 使用OddEvenFill规则：重叠的区域(中间框)会被镂空
        path.setFillRule(Qt::OddEvenFill);
        p.drawPath(path);

        // 绘制扫描框装饰 (绿色边角和文字)
        drawScanBox(p, scanRect);
    }

    // void drawScanBox(QPainter &p) {
    //     int w = width();
    //     int h = height();
    //     int boxSize = qMin(w, h) * 0.6;
    //     int x = (w - boxSize) / 2;
    //     int y = (h - boxSize) / 2;
    void drawScanBox(QPainter &p, const QRect &boxRect) {
        int x = boxRect.x();
        int y = boxRect.y();
        int boxSize = boxRect.width();
        int w = width(); // 用于文字居中

        QColor cornerColor("#00BFA5");
        QPen pen(cornerColor);
        pen.setWidth(6);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        int cornerLen = 40;
        // 左上
        p.drawLine(x, y, x + cornerLen, y);
        p.drawLine(x, y, x, y + cornerLen);
        // 右上
        p.drawLine(x + boxSize, y, x + boxSize - cornerLen, y);
        p.drawLine(x + boxSize, y, x + boxSize, y + cornerLen);
        // 左下
        p.drawLine(x, y + boxSize, x + cornerLen, y + boxSize);
        p.drawLine(x, y + boxSize, x, y + boxSize - cornerLen);
        // 右下
        p.drawLine(x + boxSize, y + boxSize, x + boxSize - cornerLen, y + boxSize);
        p.drawLine(x + boxSize, y + boxSize, x + boxSize, y + boxSize - cornerLen);

        // 文字
        QFont font = p.font();
        font.setPixelSize(36);
        font.setBold(true);
        p.setFont(font);

        QRect textRect(0, y + boxSize + 30, w, 50);
        // 阴影
        p.setPen(Qt::black);
        p.drawText(textRect.translated(2, 2), Qt::AlignCenter, "将二维码放入框内自动扫描");
        // 正文
        p.setPen(Qt::white);
        p.drawText(textRect, Qt::AlignCenter, "将二维码放入框内自动扫描");
    }

private:
    QImage m_frame;
};


class DeskControler : public QWidget
{
    Q_OBJECT

public:
    explicit DeskControler(QWidget* parent = nullptr);
    ~DeskControler();

    // ============ Kiosk模式相关 ============
    // 启用/禁用Kiosk模式
    void enableKioskMode();
    void disableKioskMode();

    // 调试退出功能: 连续快速点击5次左上角区域可退出应用
    static const int DEBUG_EXIT_TAP_COUNT = 5;          // 需要点击的次数
    static const int DEBUG_EXIT_TAP_TIMEOUT_MS = 3000;  // 点击超时时间(毫秒)
    static const int DEBUG_EXIT_TAP_AREA_SIZE = 100;    // 点击区域大小(像素)

protected:
    // 重写按键事件，拦截返回键
    void keyPressEvent(QKeyEvent *event) override;
    // 重写鼠标/触摸事件，实现调试退出
    void mousePressEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onConnectClicked();
    void onPunchHoleResponse(const QString& relayServer, int relayPort, int result);
    void onNetworkError(const QString& error);
    void onNetworkDisconnected();
    void onVideoReceiverError(const QString& error);
    void onApplicationStateChanged(Qt::ApplicationState state);

    void showMainPage(); // 显示主控页
    void onShutdownClicked(); // 关机按钮槽

    void toggleTopMenu(); // 控制下拉动画的槽函数

private:
    void setupVideoSession(const QString& relayServer, quint16 relayPort, const QString& status);
    void destroyVideoSession();
    void destroyVideoWidget();

    void loadConfig();
    void saveConfig();

    void initCamera();
    void startCamera();
    void stopCamera();

    bool connectToWiFi(const QString& ssid, const QString& password);

    void initCustomUI();
    void applyCustomStyles();

private slots:
    void handleCameraImage(const QImage &image);
    void handleCloseBtnClicked();

    void on_pushButton_camera_clicked();

private:
    Ui::DeskControlerClass ui;
    NetworkManager* m_networkManager;
    VideoReceiver* m_videoReceiver;
    QScrollArea *m_scrollArea;
    // RemoteClipboard* m_remoteClipboard = nullptr;

    QString m_dir = QString();

    AndroidVideoSurface *m_videoSurface = nullptr;
    QCamera *m_camera = nullptr;
    //QLabel *m_cameraLabel = nullptr;
    CameraPreviewWidget *m_cameraLabel = nullptr;

    // 解码状态锁
    std::atomic<bool> m_isDecoding{false};
    QZXing m_decoder;

    // ============ 连接信息成员变量 (替代UI输入框) ============
    QString m_serverIp;
    quint16 m_serverPort = 0;
    QString m_uuid;

    // ============ Kiosk模式相关成员变量 ============
    bool m_kioskModeEnabled = false;        // Kiosk模式是否启用
    int m_debugExitTapCount = 0;            // 调试退出点击计数
    QElapsedTimer m_debugExitTimer;         // 调试退出计时器
    int m_kioskRetryCount = 0;              // Kiosk模式启用重试计数

    QStackedLayout *m_mainStack = nullptr; // 页面堆叠管理器
    QWidget *m_pageStartup = nullptr;      // 页面1: 启动页
    QWidget *m_pageMain = nullptr;         // 页面2: 主控页
    QWidget *m_pageCamera = nullptr;       // 页面3: 相机页

    QVBoxLayout *m_cameraLayout = nullptr; // 相机页的布局容器

    QWidget *m_slidingMenu = nullptr;       // 实际滑动的菜单控件
    QPropertyAnimation *m_menuAnim = nullptr; // 动画控制器
    bool m_isMenuOpen = false;              // 记录菜单当前状态
};
