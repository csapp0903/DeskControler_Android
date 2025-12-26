#pragma once

#include <QWidget>
#include <QCamera>
#include <QScrollArea>

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
};
