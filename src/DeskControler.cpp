#include "DeskControler.h"
#include <QScrollArea>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QScreen>
#include <QStandardPaths>
#include <QScrollBar>
#include <QMessageBox>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>

#include <QtAndroidExtras/QtAndroid>
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroidExtras/QAndroidJniEnvironment>

#include "VideoWidget.h"
#include "LogWidget.h"

#define VIEW_SIZE QSize(1920, 1080)

DeskControler::DeskControler(QWidget* parent)
    : QWidget(parent),
    m_networkManager(nullptr),
    m_videoReceiver(nullptr),
    m_scrollArea(nullptr)
{
    ui.setupUi(this);
    // 将 LogWidget 放到界面上
    LogWidget::instance()->init(ui.widget);
    // 使用 Android 应用私有目录存储

#ifdef Q_OS_ANDROID
    m_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    LogWidget::instance()->addLog("QGui platform Q_OS_ANDROID.", LogWidget::Info);
#endif

#ifdef Q_OS_WIN
    m_dir = QCoreApplication::applicationDirPath();
    LogWidget::instance()->addLog("QGui platform Q_OS_WIN.", LogWidget::Info);
#endif
    loadConfig();

    // 当点击按钮时，发起 TCP 连接并发送 PunchHoleRequest
    connect(ui.pushButton, &QPushButton::clicked, this, &DeskControler::onConnectClicked);

    initCamera();

    // 应用重新激活
    connect(qApp, &QGuiApplication::applicationStateChanged, this, &DeskControler::onApplicationStateChanged);

    // ============ 启用Kiosk模式 ============
    // 注意: 如需禁用Kiosk模式,请注释掉下面这行
    // 延迟启用，等待Activity完全初始化
#ifdef Q_OS_ANDROID
    QTimer::singleShot(1000, this, [this]() {
        enableKioskMode();
    });
#endif
}

DeskControler::~DeskControler()
{

}

void DeskControler::loadConfig()
{
    QFile file(m_dir + "/DeskControler.json");
    QJsonObject config;
    bool valid = false;

    if (file.exists())
    {
        if (file.open(QIODevice::ReadOnly))
        {
            QByteArray data = file.readAll();
            file.close();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isNull() && doc.isObject())
            {
                config = doc.object();
                valid = true;
            }
        }
    }
    // 如果文件不存在或格式错误，则使用默认值并重写配置文件
    if (!valid)
    {
        config["server"] = QJsonObject{
            {"ip", "127.0.0.1"},
            {"port", 21116}
        };
        config["uuid"] = "";

        if (file.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(config);
            file.write(doc.toJson());
            file.close();
        }
    }

    // 从配置中读取数据
    QJsonObject serverObj = config["server"].toObject();
    QString ip = serverObj["ip"].toString("127.0.0.1");
    int port = serverObj["port"].toInt(21116);
    QString uuid = config["uuid"].toString("");

    // 设置 UI 控件
    ui.ipLineEdit_->setText(ip);
    ui.portLineEdit_->setText(QString::number(port));
    ui.lineEdit->setText(uuid);
}

void DeskControler::saveConfig()
{
    QJsonObject config;
    QJsonObject serverObj;
    serverObj["ip"] = ui.ipLineEdit_->text().trimmed();
    serverObj["port"] = ui.portLineEdit_->text().toInt();
    config["server"] = serverObj;
    config["uuid"] = ui.lineEdit->text().trimmed();

    QJsonDocument doc(config);
    QFile file(m_dir + "/DeskControler.json");
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(doc.toJson());
        file.close();
    }
}

void DeskControler::initCamera()
{
    // QR Code二维码
    m_decoder.setDecoder(QZXing::DecoderFormat_QR_CODE);
    m_decoder.setSourceFilterType(QZXing::SourceFilter_ImageNormal);
    m_decoder.setTryHarderBehaviour(QZXing::TryHarderBehaviour_ThoroughScanning | QZXing::TryHarderBehaviour_Rotate);

    // 创建自定义表面
    m_videoSurface = new AndroidVideoSurface(this);
    connect(m_videoSurface, &AndroidVideoSurface::CameraImage, this, &DeskControler::handleCameraImage);

    // 初始化摄像头
    m_camera = new QCamera(QCamera::BackFace, this);
    // 使用自定义表面替代QCameraViewfinder
    m_camera->setViewfinder(m_videoSurface);

    // 强制设置Android兼容的参数
    QCameraViewfinderSettings settings;
    settings.setResolution(1280, 720);
    // settings.setResolution(640, 480);
    settings.setPixelFormat(QVideoFrame::Format_NV21);  // Android最常用格式
    settings.setMinimumFrameRate(15.0);
    // 连续自动对焦
    m_camera->focus()->setFocusMode(QCameraFocus::ContinuousFocus);
    m_camera->setViewfinderSettings(settings);

    // 摄像头全屏显示
    m_cameraLabel = new QLabel();
    m_cameraLabel->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    QRect rect = QApplication::primaryScreen()->geometry();
    qDebug() << "-----geometry:" << rect;
    m_cameraLabel->setGeometry(rect);
    m_cameraLabel->setAlignment(Qt::AlignCenter);

    QPushButton *closeBtn = new QPushButton("X", m_cameraLabel);
    closeBtn->move(rect.width() - 160, 40);
    connect(closeBtn, &QPushButton::clicked, this, [this](){
        m_cameraLabel->close();
        stopCamera();
    });
    m_cameraLabel->showFullScreen();
    m_cameraLabel->hide();
}

void DeskControler::startCamera()
{
    qDebug() << __FUNCTION__ << "m_camera state:" << m_camera->state();
    if (m_camera->state() != QCamera::ActiveState)
    {
        // 启动摄像头
        m_cameraLabel->setWindowFlag(Qt::WindowStaysOnTopHint);
        m_cameraLabel->show();
        m_camera->start();

        ui.pushButton_camera->setText("StopCamera");
    }
}

void DeskControler::stopCamera()
{
    qDebug() << __FUNCTION__ << "m_camera state:" << m_camera->state();
    if (m_camera->state() == QCamera::ActiveState)
    {
        m_cameraLabel->hide();
        ui.pushButton_camera->setText("StartCamera");
        m_camera->stop();
    }
}

bool DeskControler::connectToWiFi(const QString &ssid, const QString &password)
{
    //<uses-permission android:name="android.permission.ACCESS_WIFI_STATE"/>
    //<uses-permission android:name="android.permission.CHANGE_WIFI_STATE"/>
    //<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION"/>

    //QtAndroid::PermissionResultMap result = QtAndroid::requestPermissionsSync(
    //    {"android.permission.ACCESS_WIFI_STATE",
    //     "android.permission.CHANGE_WIFI_STATE",
    //     "android.permission.ACCESS_FINE_LOCATION"}
    //    );
	//
    //if (result["android.permission.ACCESS_WIFI_STATE"] == QtAndroid::PermissionResult::Denied)
    //{
    //    LogWidget::instance()->addLog(QString("QtAndroid ACCESS_WIFI_STATE Permission Denied!"), LogWidget::Error);
    //    return false;
    //}

    QAndroidJniObject jsSsid = QAndroidJniObject::fromString(ssid);
    QAndroidJniObject jsPassword = QAndroidJniObject::fromString(password);
    if (!jsSsid.isValid() || !jsPassword.isValid())
    {
        LogWidget::instance()->addLog(QString("QString To QAndroidJniObject Error!"), LogWidget::Error);
        qDebug() << __FUNCTION__ << "QString To QAndroidJniObject Error!";
        return false;
    }

    bool ret = QAndroidJniObject::callStaticMethod<jboolean>(
        "org/qtproject/example/DeskControler/WiFiHelper",
        "connectToWiFi",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Z",
        QtAndroid::androidActivity().object(),
        jsSsid.object(),
        jsPassword.object()
        );

    if (ret)
    {
        LogWidget::instance()->addLog(QString("WiFi Connection Request Sent!"), LogWidget::Error);
        qDebug() << __FUNCTION__ << "WiFi Connection Request Sent!";
    }
    else
    {
        LogWidget::instance()->addLog(QString("QAndroidJniObject CallStaticMethod Error!"), LogWidget::Error);
        qDebug() << __FUNCTION__ << "QAndroidJniObject CallStaticMethod Error!";
    }

    return ret;
}

void DeskControler::handleCameraImage(const QImage &image)
{
    QPixmap pixmap = QPixmap::fromImage(image);
    m_cameraLabel->setPixmap(pixmap);

    QString info = QString::fromLocal8Bit(m_decoder.decodeImage(image).toLatin1());
    LogWidget::instance()->addLog(QString("Camera Info:%1").arg(info), LogWidget::Info);
    if (info.contains(";;"))
    {
        QMap<QString, QString> result;
        QStringList groups = info.split(";;", Qt::SkipEmptyParts);
        for (const QString &group : groups)
        {
            QStringList keyValue = group.split(":");
            if (keyValue.size() >= 2)
            {
                QString key = keyValue[0].trimmed().toUpper();
                QString value = keyValue[1].trimmed();

                // 存储到结果映射中
                result.insert(key, value);
            }
        }

        if (result.contains("WIFI") && result.contains("P")
            && result.contains("UUID") && result.contains("IP"))
        {
            ui.ipLineEdit_->setText(result["IP"]);
            ui.lineEdit->setText(result["UUID"]);
            ui.portLineEdit_->setText(result["PORT"]);

            QString ssid = result["WIFI"];
            QString password = result["P"];

            stopCamera();
            qDebug() << "-----Start ConnectToWiFi:" << ssid << password;
            LogWidget::instance()->addLog(QString("Start ConnectToWiFi:%1").arg(ssid), LogWidget::Info);

            connectToWiFi(ssid, password);
            QThread::msleep(3000);

            qDebug() << "-----Start ConnectToServer:";
            LogWidget::instance()->addLog(QString("Start ConnectToServer"), LogWidget::Info);
            onConnectClicked();
        }

        qDebug() << "-----info:" << result;
    }
}

void DeskControler::handleCloseBtnClicked()
{
    qDebug() << "-----VideoWidget Closed";
    destroyVideoWidget();
}

void DeskControler::on_pushButton_camera_clicked()
{
    if (m_camera->state() == QCamera::ActiveState)
    {
        stopCamera();
    }
    else
    {
        startCamera();
    }
}

void DeskControler::onConnectClicked()
{
    QString ip = ui.ipLineEdit_->text();
    quint16 port = ui.portLineEdit_->text().toUShort();
    QString uuid = ui.lineEdit->text();

    if (ip.isEmpty() || uuid.isEmpty() || port == 0)
    {
        LogWidget::instance()->addLog("please check IP / Port / Uuid", LogWidget::Warning);
        return;
    }

    saveConfig();
    LogWidget::instance()->addLog(
        QString("Attempting to connect to server at %1:%2 with UUID: %3").arg(ip).arg(port).arg(uuid),
        LogWidget::Info);

    m_networkManager = new NetworkManager(this);

    // 当收到 NetworkManager 的 PunchHoleResponse 信号时调用本类槽
    connect(m_networkManager, &NetworkManager::punchHoleResponseReceived,
            this, &DeskControler::onPunchHoleResponse);
    // 网络出错
    connect(m_networkManager, &NetworkManager::networkError,
            this, &DeskControler::onNetworkError);
    // 连接断开
    connect(m_networkManager, &NetworkManager::disconnected,
            this, &DeskControler::onNetworkDisconnected);

    if (!m_networkManager->connectToServer(ip, port))
    {
        m_networkManager->cleanup();
        delete m_networkManager;
        m_networkManager = nullptr;
        return;
    }

    ui.ipLineEdit_->setEnabled(false);
    ui.portLineEdit_->setEnabled(false);
    ui.lineEdit->setEnabled(false);
    ui.pushButton->setEnabled(false);

    LogWidget::instance()->addLog("Server connection established. Sending punch hole request.", LogWidget::Info);
    // 连接成功后，发送 PunchHoleRequest
    m_networkManager->sendPunchHoleRequest(uuid);
}

void DeskControler::onPunchHoleResponse(const QString& relayServer, int relayPort, int result)
{
    QString resultStr;
    switch (result)
    {
    case 0:
        resultStr = "OK";
        break;
    case 1:
        resultStr = "ID_NOT_EXIST";
        break;
    case 2:
        resultStr = "DESKSERVER_OFFLINE";
        break;
    case 3:
        resultStr = "RELAYSERVER_OFFLINE";
        break;
    default:
        resultStr = "INNER_ERROR";
        break;
    }

    LogWidget::LogLevel logLevel = (result == 0) ? LogWidget::Info : LogWidget::Error;

    LogWidget::instance()->addLog(
        QString("Punch hole response: %1 (Relay Server: %2, Port: %3)")
            .arg(resultStr).arg(relayServer).arg(relayPort),
        logLevel);

    if (result != 0)
    {
        ui.ipLineEdit_->setEnabled(true);
        ui.portLineEdit_->setEnabled(true);
        ui.lineEdit->setEnabled(true);
        ui.pushButton->setEnabled(true);
        return;
    }

    setupVideoSession(relayServer, relayPort, resultStr);
}

void DeskControler::setupVideoSession(const QString& relayServer, quint16 relayPort, const QString& status)
{
    LogWidget::instance()->addLog(
        QString("Establishing video session via relay server %1:%2 - Status: %3").arg(relayServer).arg(relayPort).arg(status),
        LogWidget::Info);

    VideoWidget* videoWidget = new VideoWidget();
    videoWidget->setAttribute(Qt::WA_DeleteOnClose, true);

    QScrollArea* scrollArea = new QScrollArea(this);
    // scrollArea->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_scrollArea = scrollArea;
#ifdef Q_OS_ANDROID
    scrollArea->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
#else
    scrollArea->setWindowFlags(Qt::Window);
#endif
    scrollArea->setWidget(videoWidget);
    // scrollArea->setAlignment(Qt::AlignCenter);
    scrollArea->setAttribute(Qt::WA_DeleteOnClose, true);
    scrollArea->viewport()->setStyleSheet("background-color: white;"); //black
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->horizontalScrollBar()->setEnabled(false);
    scrollArea->verticalScrollBar()->setEnabled(false);
    scrollArea->horizontalScrollBar()->hide();
    scrollArea->verticalScrollBar()->hide();

    connect(scrollArea, &QObject::destroyed, this, [this]() {
        destroyVideoSession();
        LogWidget::instance()->addLog("Video widget closed by user.", LogWidget::Info);
    });

    scrollArea->resize(VIEW_SIZE);
    scrollArea->raise();
    scrollArea->show();

    m_videoReceiver = new VideoReceiver(this);
    connect(m_videoReceiver, &VideoReceiver::networkError, this, &DeskControler::onVideoReceiverError);

    // m_remoteClipboard = new RemoteClipboard(this);
    // m_remoteClipboard->setRemoteWindow(scrollArea);

    // if (m_remoteClipboard->start())
    // {
    //     LogWidget::instance()->addLog("Global keyboard hook installed", LogWidget::Info);
    // }
    // else
    // {
    //     LogWidget::instance()->addLog("Failed to install global keyboard hook", LogWidget::Error);
    // }

    // connect(m_remoteClipboard, &RemoteClipboard::ctrlCPressed,
    //         m_videoReceiver, &VideoReceiver::clipboardDataCaptured);

    connect(videoWidget, &VideoWidget::mouseEventCaptured,
            m_videoReceiver, &VideoReceiver::mouseEventCaptured);

    connect(videoWidget, &VideoWidget::touchEventCaptured,
            m_videoReceiver, &VideoReceiver::touchEventCaptured);

    QObject::connect(videoWidget, &VideoWidget::keyEventCaptured,
                     m_videoReceiver, &VideoReceiver::keyEventCaptured);

    connect(videoWidget, &VideoWidget::closeBtnClicked, this, &DeskControler::handleCloseBtnClicked);

    // QObject::connect(m_videoReceiver, &VideoReceiver::onClipboardMessageReceived,
    //                  m_remoteClipboard, &RemoteClipboard::onClipboardMessageReceived);

    static bool firstFrame = true;
    firstFrame = true;
    connect(m_videoReceiver, &VideoReceiver::frameReady, this, [videoWidget, scrollArea](const QImage& img) {
        static QSize imgSize = img.size();
        static QSize newSize = imgSize;

        if (firstFrame && !img.isNull())
        {
            LogWidget::instance()->addLog("Video stream started and UI initialized.", LogWidget::Info);

            QScreen* screen = QGuiApplication::primaryScreen();
            QSize screenSize = VIEW_SIZE;

            qreal scale = 1.0;
            qreal offsetX = 0, offsetY = 0;

            QSize border = QSize(4,4);
            if (screen)
            {
                screenSize = screen->size();

                /* wTest */
                //QSize initialSize(1920, 1200);
                //screenSize = initialSize;

                scale = qMin(screenSize.width() * 1.0 / imgSize.width(), screenSize.height() * 1.0 / imgSize.height());
                newSize = imgSize * scale;
                offsetX = (imgSize.width() - newSize.width()) / 2;
                offsetY = (imgSize.height() - newSize.height()) / 2;

                QString str = QString("UI initialized scale:%1 offsetX:%2 offsetY:%3 newSize w:%4 h:%5")
                                  .arg(scale).arg(offsetX).arg(offsetY).arg(newSize.width()).arg(newSize.height());
                LogWidget::instance()->addLog(str, LogWidget::Info);
                qDebug() <<"DeskControler VideoWidget Size:" << imgSize << screenSize << newSize << scale << offsetX << offsetY;
            }

            videoWidget->setPreValue(scale);
            scrollArea->resize(screenSize+border);
            //videoWidget->move(offsetX, offsetY);
            firstFrame = false;

            /* wTest */
            //scrollArea->move(offsetX-2560, 120);
        }

        QImage newImg = img;
        if (newSize != imgSize)
        {
            // 平滑 画质
            newImg = img.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        videoWidget->setFrame(newImg);
    });

    QString uuid = ui.lineEdit->text();
    m_videoReceiver->startConnect(relayServer, static_cast<quint16>(relayPort), uuid);
}

void DeskControler::destroyVideoSession()
{
    // if (m_remoteClipboard)
    // {
    //     m_remoteClipboard->stop();
    //     delete m_remoteClipboard;
    //     m_remoteClipboard = nullptr;
    // }

    ui.pushButton->setEnabled(true);
    ui.ipLineEdit_->setEnabled(true);
    ui.portLineEdit_->setEnabled(true);
    ui.lineEdit->setEnabled(true);

    if (m_videoReceiver)
    {
        m_videoReceiver->stopReceiving();
        delete m_videoReceiver;
        m_videoReceiver = nullptr;
    }
    if (m_networkManager)
    {
        m_networkManager->cleanup();
        delete m_networkManager;
        m_networkManager = nullptr;
    }
}

void DeskControler::destroyVideoWidget()
{
    if (m_scrollArea)
    {
        m_scrollArea->hide();
        m_scrollArea->deleteLater();
        m_scrollArea = nullptr;
    }
}

void DeskControler::onNetworkError(const QString& error)
{
    LogWidget::instance()->addLog(QString("NetworkError %1").arg(error), LogWidget::Warning);
}

void DeskControler::onNetworkDisconnected()
{
    LogWidget::instance()->addLog("Network connection disconnected.", LogWidget::Warning);
    // 如果需要，恢复按钮可点击
    ui.pushButton->setEnabled(true);
}

void DeskControler::onVideoReceiverError(const QString &error)
{
    LogWidget::instance()->addLog("VideoReceiver error: " + error, LogWidget::Warning);

    if (m_scrollArea)
    {
        destroyVideoWidget();
        QMessageBox::critical(this, "Error", "VideoReceiver error: " + error);
    }
}

void DeskControler::onApplicationStateChanged(Qt::ApplicationState state)
{
    LogWidget::instance()->addLog(QString("onApplicationStateChanged %1").arg(state), LogWidget::Warning);
    if (state == Qt::ApplicationActive && m_scrollArea)
    {
#ifdef Q_OS_ANDROID
        m_scrollArea->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
#endif
        // m_scrollArea->show();
        m_scrollArea->raise();
    }

    // Kiosk模式: 应用激活时重新隐藏系统UI
#ifdef Q_OS_ANDROID
    if (state == Qt::ApplicationActive && m_kioskModeEnabled)
    {
        QAndroidJniObject::callStaticMethod<void>(
            "org/qtproject/example/DeskControler/KioskHelper",
            "hideSystemUI",
            "(Landroid/app/Activity;)V",
            QtAndroid::androidActivity().object()
        );
    }
#endif
}

// ============ Kiosk模式实现 ============

void DeskControler::enableKioskMode()
{
#ifdef Q_OS_ANDROID
    LogWidget::instance()->addLog("启用Kiosk模式", LogWidget::Info);

    // 检查Activity是否有效
    QAndroidJniObject activity = QtAndroid::androidActivity();
    if (!activity.isValid())
    {
        LogWidget::instance()->addLog("Kiosk模式: Activity无效，稍后重试", LogWidget::Warning);
        // 延迟重试
        QTimer::singleShot(500, this, [this]() {
            enableKioskMode();
        });
        return;
    }

    m_kioskModeEnabled = true;

    // 调用Android端的Kiosk模式
    QAndroidJniObject::callStaticMethod<void>(
        "org/qtproject/example/DeskControler/KioskHelper",
        "enableKioskMode",
        "(Landroid/app/Activity;)V",
        activity.object()
    );

    // 检查JNI异常
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        LogWidget::instance()->addLog("Kiosk模式: JNI调用异常", LogWidget::Error);
        m_kioskModeEnabled = false;
        return;
    }

    LogWidget::instance()->addLog("Kiosk模式已启用 - 调试退出: 快速点击左上角5次", LogWidget::Info);
#endif
}

void DeskControler::disableKioskMode()
{
#ifdef Q_OS_ANDROID
    LogWidget::instance()->addLog("禁用Kiosk模式", LogWidget::Info);

    m_kioskModeEnabled = false;

    // 调用Android端禁用Kiosk模式
    QAndroidJniObject::callStaticMethod<void>(
        "org/qtproject/example/DeskControler/KioskHelper",
        "disableKioskMode",
        "(Landroid/app/Activity;)V",
        QtAndroid::androidActivity().object()
    );
#endif
}

void DeskControler::keyPressEvent(QKeyEvent *event)
{
#ifdef Q_OS_ANDROID
    // Kiosk模式: 拦截返回键
    if (m_kioskModeEnabled && event->key() == Qt::Key_Back)
    {
        LogWidget::instance()->addLog("Kiosk模式: 返回键已被拦截", LogWidget::Info);
        event->accept();
        return;
    }
#endif

    QWidget::keyPressEvent(event);
}

void DeskControler::mousePressEvent(QMouseEvent *event)
{
    // ============ 调试退出功能 ============
    // 在左上角区域连续快速点击5次可退出应用
    // 注意: 此功能仅供调试使用，正式发布时应删除此段代码

#ifdef Q_OS_ANDROID
    if (m_kioskModeEnabled)
    {
        QPoint pos = event->pos();

        // 检查是否点击在左上角区域
        if (pos.x() < DEBUG_EXIT_TAP_AREA_SIZE && pos.y() < DEBUG_EXIT_TAP_AREA_SIZE)
        {
            // 检查是否超时
            if (!m_debugExitTimer.isValid() || m_debugExitTimer.elapsed() > DEBUG_EXIT_TAP_TIMEOUT_MS)
            {
                // 重新开始计数
                m_debugExitTapCount = 1;
                m_debugExitTimer.start();
                LogWidget::instance()->addLog(QString("调试退出: 点击 %1/%2").arg(m_debugExitTapCount).arg(DEBUG_EXIT_TAP_COUNT), LogWidget::Info);
            }
            else
            {
                m_debugExitTapCount++;
                LogWidget::instance()->addLog(QString("调试退出: 点击 %1/%2").arg(m_debugExitTapCount).arg(DEBUG_EXIT_TAP_COUNT), LogWidget::Info);

                if (m_debugExitTapCount >= DEBUG_EXIT_TAP_COUNT)
                {
                    LogWidget::instance()->addLog("调试退出: 触发退出!", LogWidget::Warning);

                    // 调用Android端退出
                    QAndroidJniObject::callStaticMethod<void>(
                        "org/qtproject/example/DeskControler/KioskHelper",
                        "exitApp",
                        "(Landroid/app/Activity;)V",
                        QtAndroid::androidActivity().object()
                    );

                    // 也在Qt层面退出
                    qApp->quit();
                }
            }
        }
    }
#endif

    QWidget::mousePressEvent(event);
}
