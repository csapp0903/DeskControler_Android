#pragma once

#include <QWidget>
#include <QCamera>
#include <QScrollArea>
#include <QElapsedTimer>

#include "NetworkManager.h"
#include "VideoReceiver.h"
#include "ui_DeskControler.h"
// #include "RemoteClipboard.h"

#include "AndroidVideoSurface.h"
#include "QZXing.h"

class NetworkManager;

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
    // 注意: 此功能仅供调试使用，正式发布时应删除
    static const int DEBUG_EXIT_TAP_COUNT = 5;          // 需要点击的次数
    static const int DEBUG_EXIT_TAP_TIMEOUT_MS = 3000;  // 点击超时时间(毫秒)
    static const int DEBUG_EXIT_TAP_AREA_SIZE = 100;    // 点击区域大小(像素)

protected:
    // 重写按键事件，拦截返回键
    void keyPressEvent(QKeyEvent *event) override;
    // 重写鼠标/触摸事件，实现调试退出
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void onConnectClicked();
    void onPunchHoleResponse(const QString& relayServer, int relayPort, int result);
    void onNetworkError(const QString& error);
    void onNetworkDisconnected();
    void onVideoReceiverError(const QString& error);
    void onApplicationStateChanged(Qt::ApplicationState state);

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
    QLabel *m_cameraLabel = nullptr;
    QZXing m_decoder;

    // ============ Kiosk模式相关成员变量 ============
    bool m_kioskModeEnabled = false;        // Kiosk模式是否启用
    int m_debugExitTapCount = 0;            // 调试退出点击计数
    QElapsedTimer m_debugExitTimer;         // 调试退出计时器
};
