#include "DeskControler.h"
#include <QScrollArea>
#include <QPushButton>
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
#include <QPainter>
#include <QCoreApplication>
#include <QtNetwork/QTcpSocket>

#include <QtAndroidExtras/QtAndroid>
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroidExtras/QAndroidJniEnvironment>

#include "VideoWidget.h"
#include "LogWidget.h"

#define VIEW_SIZE QSize(1920, 1080)

// ==========================================
// [配置]
// ==========================================
const QString COLOR_BG_MAIN     = "#FFFFFF";  // 背景
const QString COLOR_THEME_BLUE  = "#4472C4";  // 主色调
const QString COLOR_TEXT_TITLE  = "#203764";  // 标题色
const QString COLOR_TEXT_BTN    = "#FFFFFF";  // 按钮文字
const QString COLOR_BORDER      = "#4472C4";  // 窗口边框

// 字体大小配置
const QString FONT_TITLE_EN     = "24px";     // 英文标题大小
const QString FONT_TITLE_CN     = "48px";     // 中文标题大小 (加粗)
const QString FONT_BTN          = "20px";     // 按钮文字大小

DeskControler::DeskControler(QWidget* parent)
    : QWidget(parent),
    m_networkManager(nullptr),
    m_videoReceiver(nullptr),
    m_scrollArea(nullptr),
    m_menuAnim(nullptr),
    m_slidingMenu(nullptr)
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
    //connect(ui.pushButton, &QPushButton::clicked, this, &DeskControler::onConnectClicked);
    // 当点击按钮时，启动相机扫码
    connect(ui.pushButton, &QPushButton::clicked, this, &DeskControler::startCamera);

    initCamera();

    initCustomUI();
    applyCustomStyles();

    connect(ui.pushButton, &QPushButton::clicked, this, &DeskControler::onConnectClicked);
    // 应用重新激活
    connect(qApp, &QGuiApplication::applicationStateChanged, this, &DeskControler::onApplicationStateChanged);

// ============ 启用Kiosk模式 ============
// 如需禁用Kiosk模式,请注释掉下面这行
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

void DeskControler::initCustomUI()
{
    // 清理旧布局
    if (this->layout()) delete this->layout();

    this->setObjectName("MainWindow");

    // 创建堆叠布局
    m_mainStack = new QStackedLayout(this);
    m_mainStack->setStackingMode(QStackedLayout::StackOne);

    // ==========================================
    // 初始待机页
    // ==========================================
    m_pageStartup = new QWidget(this);
    QVBoxLayout *layoutStartup = new QVBoxLayout(m_pageStartup);
    layoutStartup->setContentsMargins(40, 40, 40, 40);

    // Logo
    QLabel *lbLogo1 = new QLabel(m_pageStartup);
    lbLogo1->setObjectName("BrandLogo");
    lbLogo1->setFixedSize(200, 60);

    // 标题
    QWidget *titleContainer1 = new QWidget(m_pageStartup);
    QVBoxLayout *titleLayout1 = new QVBoxLayout(titleContainer1);

    QLabel *lbTitleEn1 = new QLabel("Cardio Space™", titleContainer1);
    lbTitleEn1->setAlignment(Qt::AlignCenter);
    lbTitleEn1->setStyleSheet("color: " + COLOR_TEXT_TITLE + "; font-size: " + FONT_TITLE_EN + ";");

    QLabel *lbTitleCn1 = new QLabel("血管内超声诊断系统", titleContainer1);
    lbTitleCn1->setAlignment(Qt::AlignCenter);
    lbTitleCn1->setStyleSheet("color: " + COLOR_TEXT_TITLE + "; font-size: " + FONT_TITLE_CN + "; font-weight: bold;");

    titleLayout1->addWidget(lbTitleEn1);
    titleLayout1->addWidget(lbTitleCn1);

    layoutStartup->addWidget(lbLogo1, 0, Qt::AlignLeft | Qt::AlignTop);
    layoutStartup->addStretch();
    layoutStartup->addWidget(titleContainer1, 0, Qt::AlignCenter);
    layoutStartup->addStretch();

    QTimer::singleShot(3000, this, &DeskControler::showMainPage);


    // ==========================================
    // 连接控制页
    // ==========================================
    m_pageMain = new QWidget(this);
    QVBoxLayout *layoutMain = new QVBoxLayout(m_pageMain);
    layoutMain->setContentsMargins(40, 40, 40, 40);

    // Logo
    QLabel *lbLogo2 = new QLabel(m_pageMain);
    lbLogo2->setObjectName("BrandLogo");
    lbLogo2->setFixedSize(200, 60);

    // 中间容器
    QWidget *centerGroup = new QWidget(m_pageMain);
    QVBoxLayout *centerLayout = new QVBoxLayout(centerGroup);
    centerLayout->setSpacing(25); // 调整间距

    // 标题块
    QWidget *titleBlock = new QWidget(centerGroup);
    QVBoxLayout *tbLayout = new QVBoxLayout(titleBlock);
    QLabel *lbTitleEn2 = new QLabel("Cardio Space™", titleBlock);
    lbTitleEn2->setAlignment(Qt::AlignCenter);
    lbTitleEn2->setStyleSheet("color: " + COLOR_TEXT_TITLE + "; font-size: " + FONT_TITLE_EN + ";");
    QLabel *lbTitleCn2 = new QLabel("血管内超声诊断系统", titleBlock);
    lbTitleCn2->setAlignment(Qt::AlignCenter);
    lbTitleCn2->setStyleSheet("color: " + COLOR_TEXT_TITLE + "; font-size: " + FONT_TITLE_CN + "; font-weight: bold;");
    tbLayout->addWidget(lbTitleEn2);
    tbLayout->addWidget(lbTitleCn2);

    // // 输入框块
    // QWidget *inputBlock = new QWidget(centerGroup);
    // QVBoxLayout *inputLayout = new QVBoxLayout(inputBlock);
    // inputLayout->setSpacing(10);

    // // 设置占位符和默认值
    // ui.ipLineEdit_->setPlaceholderText("服务器 IP 地址");
    // ui.portLineEdit_->setPlaceholderText("端口号");
    // ui.lineEdit->setPlaceholderText("设备 UUID");

    // ui.ipLineEdit_->setText("127.0.0.1");
    // ui.portLineEdit_->setText("21116");

    // // 设置宽度
    // int inputWidth = 320;
    // ui.ipLineEdit_->setFixedWidth(inputWidth);
    // ui.portLineEdit_->setFixedWidth(inputWidth);
    // ui.lineEdit->setFixedWidth(inputWidth);

    // ui.ipLineEdit_->setParent(inputBlock);
    // ui.portLineEdit_->setParent(inputBlock);
    // ui.lineEdit->setParent(inputBlock);

    // inputLayout->addWidget(ui.ipLineEdit_);
    // inputLayout->addWidget(ui.portLineEdit_);
    // inputLayout->addWidget(ui.lineEdit);
    // inputLayout->setAlignment(Qt::AlignCenter);

    // // ==========================================
    // // 扫码按钮块 (放在输入框下面)
    // // ==========================================
    // // 复用 ui.pushButton_camera (StartCamera)
    // ui.pushButton_camera->setText("扫码配置");
    // ui.pushButton_camera->setObjectName("BtnScan"); // 设置特殊样式 ID
    // ui.pushButton_camera->setFixedSize(inputWidth, 45); // 宽度与输入框一致
    // ui.pushButton_camera->setCursor(Qt::PointingHandCursor);
    // ui.pushButton_camera->setVisible(true); // 确保显示
    // ui.pushButton_camera->setParent(centerGroup); // 挂载到布局

    // 底部按钮块 (连接/关机)
    QWidget *btnGroup = new QWidget(centerGroup);
    QHBoxLayout *btnLayout = new QHBoxLayout(btnGroup);
    btnLayout->setSpacing(40);

    ui.pushButton->setText("连接");
    ui.pushButton->setObjectName("BtnAction");
    ui.pushButton->setFixedSize(160, 50);

    QPushButton *btnShutdown = new QPushButton("关机", btnGroup);
    btnShutdown->setObjectName("BtnAction");
    btnShutdown->setFixedSize(160, 50);
    connect(btnShutdown, &QPushButton::clicked, this, &DeskControler::onShutdownClicked);

    btnLayout->addStretch();
    btnLayout->addWidget(ui.pushButton);
    btnLayout->addWidget(btnShutdown);
    btnLayout->addStretch();

    // 组装顺序：标题 -> 输入框 -> [扫码按钮] -> 连接/关机
    centerLayout->addWidget(titleBlock);
    // centerLayout->addWidget(inputBlock);
    // centerLayout->addWidget(ui.pushButton_camera, 0, Qt::AlignCenter); // 居中添加扫码按钮
    centerLayout->addWidget(btnGroup);

    // 隐藏多余的调试按钮
    ui.pushButton_2->setVisible(false);
    ui.pushButton_2->setParent(m_pageMain);

    layoutMain->addWidget(lbLogo2, 0, Qt::AlignLeft | Qt::AlignTop);
    layoutMain->addStretch();
    layoutMain->addWidget(centerGroup, 0, Qt::AlignCenter);
    layoutMain->addStretch();

    // ==========================================
    // 视频播放页
    // ==========================================
    m_pageCamera = new QWidget(this);
    QVBoxLayout *layoutCam = new QVBoxLayout(m_pageCamera);
    layoutCam->setContentsMargins(0, 0, 0, 0);
    layoutCam->setSpacing(0);

    // 顶部容器
    // 它的作用是定义“显示区域”，超出它的子控件部分会被裁剪(视觉上)
    QWidget *topContainer = new QWidget(m_pageCamera);
    topContainer->setObjectName("TopContainer");
    topContainer->setFixedHeight(120);
    topContainer->setStyleSheet("background-color: black;");
    topContainer->installEventFilter(this);    // 点击这里触发下拉

    // 滑动菜单 (实际移动的部分)
    // topContainer 的子控件
    m_slidingMenu = new QWidget(topContainer);
    m_slidingMenu->setObjectName("SlidingMenu");
    // 初始位置设为 (0, -120)，即完全隐藏在顶部上方
    m_slidingMenu->setGeometry(0, -120, 1920, 120);
    m_slidingMenu->setStyleSheet("background-color: transparent;"); // 透明背景

    // 菜单内容布局
    QHBoxLayout *menuLayout = new QHBoxLayout(m_slidingMenu);
    menuLayout->setAlignment(Qt::AlignCenter);
    menuLayout->setContentsMargins(0, 0, 0, 0);

    // 断开连接按钮
    QPushButton *btnExit = new QPushButton("断开连接", m_slidingMenu);
    btnExit->setObjectName("BtnExit");
    btnExit->setFixedSize(160, 50);
    btnExit->setCursor(Qt::PointingHandCursor);

    // 按钮样式
    btnExit->setStyleSheet(
        "QPushButton { background-color: #D32F2F; color: white; border-radius: 4px; font-weight: bold; font-size: 18px; }"
        "QPushButton:pressed { background-color: #B71C1C; }"
        );

    connect(btnExit, &QPushButton::clicked, this, &DeskControler::handleCloseBtnClicked);
    menuLayout->addWidget(btnExit);

    // 初始化动画对象
    // 动画目标是 m_slidingMenu 的 "pos" (位置) 属性
    m_menuAnim = new QPropertyAnimation(m_slidingMenu, "pos", this);
    m_menuAnim->setDuration(300); // 动画时长 300ms
    m_menuAnim->setEasingCurve(QEasingCurve::OutQuad); // 快出慢停

    // 底部视频容器
    QWidget *videoContainer = new QWidget(m_pageCamera);
    videoContainer->setObjectName("VideoContainer");
    videoContainer->setStyleSheet("background-color: black;");
    QVBoxLayout *videoLayout = new QVBoxLayout(videoContainer);
    videoLayout->setContentsMargins(0, 0, 0, 0);

    layoutCam->addWidget(topContainer);
    layoutCam->addWidget(videoContainer);

    m_mainStack->addWidget(m_pageStartup);
    m_mainStack->addWidget(m_pageMain);
    m_mainStack->addWidget(m_pageCamera);
    m_mainStack->setCurrentIndex(0);
}

void DeskControler::applyCustomStyles()
{
    QString qss = QString(
                      "QWidget#MainWindow { background-color: %1; border: 2px solid %4; }"
                      "QWidget { background-color: %1; font-family: 'Microsoft YaHei', sans-serif; }"

                      // 输入框
                      "QLineEdit { "
                      "   background-color: #F5F5F5; border: 1px solid #CCCCCC; "
                      "   border-radius: 4px; padding: 8px; font-size: 16px; color: #333333; "
                      "}"
                      "QLineEdit:focus { border: 1px solid %4; background-color: #FFFFFF; }"

                      // 动作按钮 (连接/关机/断开)
                      "QPushButton#BtnAction { "
                      "   background-color: %2; color: %3; border: none; border-radius: 2px; "
                      "   font-size: %5; font-weight: bold;"
                      "}"
                      "QPushButton#BtnAction:pressed { background-color: #35589A; }"
                      "QPushButton#BtnAction:disabled { background-color: #999999; }"

                      // 扫码配置按钮样式
                      "QPushButton#BtnScan { "
                      "   background-color: transparent; "
                      "   color: %4; "              // 字体用蓝
                      "   border: 1px solid %4; "   // 边框用蓝
                      "   border-radius: 4px; "
                      "   font-size: 16px; "
                      "}"
                      "QPushButton#BtnScan:pressed { "
                      "   background-color: #EBF2FA; " // 按下变浅蓝背景
                      "}"

                      "QLabel#BrandLogo { border: 1px dashed #CCCCCC; background-color: transparent; }"
                      )
                      .arg(COLOR_BG_MAIN)     // %1
                      .arg(COLOR_THEME_BLUE)  // %2
                      .arg(COLOR_TEXT_BTN)    // %3
                      .arg(COLOR_BORDER)      // %4
                      .arg(FONT_BTN);         // %5

    this->setStyleSheet(qss);
}

void DeskControler::showMainPage()
{
    m_mainStack->setCurrentIndex(1);
}

// void DeskControler::onShutdownClicked()
// {
//     // 关机功能暂未实现
//     QMessageBox::information(this, "Shutdown", "Shutdown function pending...");
// }
void DeskControler::onShutdownClicked()
{
#ifdef Q_OS_ANDROID
    // 调用 KioskHelper 中的 showPowerMenu 方法
    QAndroidJniObject::callStaticMethod<void>(
        "org/qtproject/example/DeskControler/KioskHelper",
        "showPowerMenu",
        "(Landroid/app/Activity;)V",
        QtAndroid::androidActivity().object()
        );
#else
    // Windows 平台或其他平台的模拟实现
    QMessageBox::information(this, "Shutdown", "Function available on Android only.");
#endif
}

void DeskControler::toggleTopMenu()
{
    if (!m_slidingMenu || !m_menuAnim) return;

    // 如果动画正在播放，忽略操作，防止鬼畜
    if (m_menuAnim->state() == QAbstractAnimation::Running) return;

    // 获取当前容器的宽度（适配屏幕宽度）
    int currentW = m_slidingMenu->parentWidget()->width();

    if (m_isMenuOpen) {
        // [当前是打开的] -> 执行收起动画
        // 从 (0, 0) 移动到 (0, -120)
        m_menuAnim->setStartValue(QPoint(0, 0));
        m_menuAnim->setEndValue(QPoint(0, -120));
        m_isMenuOpen = false;
    } else {
        // [当前是关闭的] -> 执行下拉动画
        // 从 (0, -120) 移动到 (0, 0)
        m_menuAnim->setStartValue(QPoint(0, -120));
        m_menuAnim->setEndValue(QPoint(0, 0));

        // 确保宽度适配，防止旋转屏幕后宽度不对
        m_slidingMenu->setFixedWidth(currentW);
        m_isMenuOpen = true;
    }

    m_menuAnim->start();
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

    // // 设置 UI 控件
    // ui.ipLineEdit_->setText(ip);
    // ui.portLineEdit_->setText(QString::number(port));
    // ui.lineEdit->setText(uuid);
}

void DeskControler::saveConfig()
{
    QJsonObject config;
    QJsonObject serverObj;
    // serverObj["ip"] = ui.ipLineEdit_->text().trimmed();
    // serverObj["port"] = ui.portLineEdit_->text().toInt();
    // config["server"] = serverObj;
    // config["uuid"] = ui.lineEdit->text().trimmed();
    serverObj["ip"] = m_serverIp; // ui.ipLineEdit_->text().trimmed();
    serverObj["port"] = m_serverPort; // ui.portLineEdit_->text().toInt();
    config["server"] = serverObj;
    config["uuid"] = m_uuid; // ui.lineEdit->text().trimmed();

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

    //QPushButton *closeBtn = new QPushButton("X", m_cameraLabel);
    QPushButton *closeBtn = new QPushButton("关闭相机", m_cameraLabel);
    closeBtn->setStyleSheet(
        "background-color: #D32F2F; color: white; border-radius: 4px; "
        "font-size: 16px; padding: 8px; font-weight: bold;"
        );
    closeBtn->resize(120, 50); // 调整大小
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

    // ==========================================
    // 绘制二维码辅助框
    // ==========================================
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);

    int w = pixmap.width();
    int h = pixmap.height();

    // 定义扫描框大小
    int boxSize = qMin(w, h) * 0.6;
    int x = (w - boxSize) / 2;
    int y = (h - boxSize) / 2;

    // --- 绘制扫描框四角 ---
    QColor cornerColor("#00BFA5");
    QPen pen(cornerColor);
    pen.setWidth(6); // 线条宽度
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    int cornerLen = 40; // 拐角长度
    // 左上角
    p.drawLine(x, y, x + cornerLen, y);
    p.drawLine(x, y, x, y + cornerLen);
    // 右上角
    p.drawLine(x + boxSize, y, x + boxSize - cornerLen, y);
    p.drawLine(x + boxSize, y, x + boxSize, y + cornerLen);
    // 左下角
    p.drawLine(x, y + boxSize, x + cornerLen, y + boxSize);
    p.drawLine(x, y + boxSize, x, y + boxSize - cornerLen);
    // 右下角
    p.drawLine(x + boxSize, y + boxSize, x + boxSize - cornerLen, y + boxSize);
    p.drawLine(x + boxSize, y + boxSize, x + boxSize, y + boxSize - cornerLen);

    // --- 绘制提示文字 ---
    // 增加一点阴影，防止背景太亮看不清文字
    p.setPen(Qt::black); // 阴影色
    QFont font = p.font();
    font.setPixelSize(36);
    font.setBold(true);
    p.setFont(font);

    QRect textRect(0, y + boxSize + 30, w, 50);
    // 先画阴影 (偏移 2px)
    p.drawText(textRect.translated(2, 2), Qt::AlignCenter, "将二维码放入框内自动扫描");

    // 再画正文 (白色)
    p.setPen(Qt::white);
    p.drawText(textRect, Qt::AlignCenter, "将二维码放入框内自动扫描");

    p.end();

    // 显示处理后的图像
    m_cameraLabel->setPixmap(pixmap);

    //m_cameraLabel->setPixmap(pixmap);

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
            && result.contains("UUID") && result.contains("IP") && result.contains("PORT"))
        {
            // ui.ipLineEdit_->setText(result["IP"]);
            // ui.lineEdit->setText(result["UUID"]);
            // ui.portLineEdit_->setText(result["PORT"]);
            m_serverIp = result["IP"];
            m_uuid = result["UUID"];
            m_serverPort = result.contains("PORT") ? result["PORT"].toUShort() : 21116;

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
    // QString ip = ui.ipLineEdit_->text();
    // quint16 port = ui.portLineEdit_->text().toUShort();
    // QString uuid = ui.lineEdit->text();
    QString ip = m_serverIp; // ui.ipLineEdit_->text();
    quint16 port = m_serverPort; // ui.portLineEdit_->text().toUShort();
    QString uuid = m_uuid; // ui.lineEdit->text();

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

    // ui.ipLineEdit_->setEnabled(false);
    // ui.portLineEdit_->setEnabled(false);
    // ui.lineEdit->setEnabled(false);
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
        // ui.ipLineEdit_->setEnabled(true);
        // ui.portLineEdit_->setEnabled(true);
        // ui.lineEdit->setEnabled(true);
        ui.pushButton->setEnabled(true);
        return;
    }

    setupVideoSession(relayServer, relayPort, resultStr);
}

void DeskControler::setupVideoSession(const QString& relayServer, quint16 relayPort, const QString& status)
{
    //LogWidget::instance()->addLog("Starting Video Session...", LogWidget::Info);

    // VideoWidget
    VideoWidget* videoWidget = new VideoWidget();
    //videoWidget->setWindowFlags(Qt::Widget);
    videoWidget->setAttribute(Qt::WA_DeleteOnClose, false);
    //videoWidget->setStyleSheet("background-color: black; border-image: none;");
    videoWidget->setStyleSheet("background-color: black;");

    // 插入视频
    QWidget *videoContainer = m_pageCamera->findChild<QWidget*>("VideoContainer");
    if (videoContainer && videoContainer->layout()) {
        QLayoutItem *item;
        while ((item = videoContainer->layout()->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        videoContainer->layout()->addWidget(videoWidget);
    }

    // ==========================================
    // 重置下拉菜单状态
    // ==========================================
    if (m_slidingMenu) {
        // 立即移动到隐藏位置，不播放动画
        m_slidingMenu->move(0, -120);
        m_isMenuOpen = false;

        if (m_slidingMenu->parentWidget()) {
            m_slidingMenu->setFixedWidth(m_slidingMenu->parentWidget()->width());
        }
    }

    // 切换界面
    m_mainStack->setCurrentIndex(2);

    // 连接逻辑
    m_scrollArea = nullptr;
    m_videoReceiver = new VideoReceiver(this);
    connect(m_videoReceiver, &VideoReceiver::networkError, this, &DeskControler::onVideoReceiverError);

    connect(videoWidget, &VideoWidget::mouseEventCaptured, m_videoReceiver, &VideoReceiver::mouseEventCaptured);
    connect(videoWidget, &VideoWidget::touchEventCaptured, m_videoReceiver, &VideoReceiver::touchEventCaptured);
    connect(videoWidget, &VideoWidget::keyEventCaptured, m_videoReceiver, &VideoReceiver::keyEventCaptured);

    static bool firstFrame = true;
    firstFrame = true;

    connect(m_videoReceiver, &VideoReceiver::frameReady, this, [videoWidget](const QImage& img) {
        videoWidget->setFrame(img);
        videoWidget->update();
    });

    //QString uuid = ui.lineEdit->text();
    QString uuid = m_uuid;  // 使用成员变量
    m_videoReceiver->startConnect(relayServer, static_cast<quint16>(relayPort), uuid);
}

void DeskControler::destroyVideoSession()
{
    //LogWidget::instance()->addLog("Destroying Video Session...", LogWidget::Info);

    // 恢复 UI 状态
    ui.pushButton->setEnabled(true);
    ui.pushButton->setText("连接");

    // NetworkManager
    NetworkManager* oldNetManager = m_networkManager;
    m_networkManager = nullptr;

    if (oldNetManager) {
        oldNetManager->disconnect(); // 断开所有信号
        QTimer::singleShot(100, oldNetManager, [oldNetManager](){
            oldNetManager->cleanup();
            oldNetManager->deleteLater();
        });
    }

    // VideoReceiver
    VideoReceiver* oldReceiver = m_videoReceiver;
    m_videoReceiver = nullptr; // 立即置空

    if (oldReceiver) {
        oldReceiver->disconnect(); // 立即断开 frameReady 等信号，防止刷新 UI

        QTimer::singleShot(50, oldReceiver, [oldReceiver](){
            oldReceiver->stopReceiving();
            oldReceiver->deleteLater();
        });
    }

    // 延时 500ms 后再恢复“连接”按钮
    // 点击断开 -> 界面切回 -> 按钮变灰半秒 -> 按钮变回“连接”
    QTimer::singleShot(500, this, [this](){
        ui.pushButton->setText("连接");
        ui.pushButton->setEnabled(true);
    });
}

void DeskControler::destroyVideoWidget()
{
    // 先清理底层会话
    destroyVideoSession();

    // 隐藏并重置下拉菜单
    if (m_slidingMenu) {
        m_slidingMenu->move(0, -120); // 移回顶部
        m_isMenuOpen = false;
    }

    // 清理视频 UI
    if (m_pageCamera) {
        QWidget *videoContainer = m_pageCamera->findChild<QWidget*>("VideoContainer");
        if (videoContainer) {
            QObjectList children = videoContainer->children();
            for (QObject* child : children) {
                if (child->isWidgetType()) {
                    QWidget* w = qobject_cast<QWidget*>(child);

                    // 先隐藏，停止绘图
                    w->hide();

                    // 移除布局
                    if (videoContainer->layout()) {
                        videoContainer->layout()->removeWidget(w);
                    }

                    w->deleteLater();
                }
            }
        }
    }

    // 切回主控页 (Page 1)
    if (m_mainStack) {
        m_mainStack->setCurrentIndex(1);
    }

    // 强制刷新
    this->update();
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
#endif \
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
    const int MAX_RETRY = 5;  // 最大重试次数

    //LogWidget::instance()->addLog(QString("启用Kiosk模式 (尝试 %1/%2)").arg(m_kioskRetryCount + 1).arg(MAX_RETRY), LogWidget::Info);

    // 检查Activity是否有效
    QAndroidJniObject activity = QtAndroid::androidActivity();
    if (!activity.isValid())
    {
        m_kioskRetryCount++;
        if (m_kioskRetryCount < MAX_RETRY)
        {
            //LogWidget::instance()->addLog("Kiosk模式: Activity无效，稍后重试", LogWidget::Warning);
            QTimer::singleShot(500, this, [this]() {
                enableKioskMode();
            });
        }
        else
        {
            //LogWidget::instance()->addLog("Kiosk模式: 重试次数已用尽，放弃启用", LogWidget::Error);
        }
        return;
    }

    m_kioskModeEnabled = true;
    m_kioskRetryCount = 0;  // 重置重试计数

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
        //LogWidget::instance()->addLog("Kiosk模式: JNI调用异常，Java类可能未正确加载", LogWidget::Error);
        m_kioskModeEnabled = false;
        return;
    }

    //LogWidget::instance()->addLog("Kiosk模式已启用 - 调试退出: 快速点击左上角5次", LogWidget::Info);
#endif
}

void DeskControler::disableKioskMode()
{
#ifdef Q_OS_ANDROID
    //LogWidget::instance()->addLog("禁用Kiosk模式", LogWidget::Info);

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
        //LogWidget::instance()->addLog("Kiosk模式: 返回键已被拦截", LogWidget::Info);
        event->accept();
        return;
    }
#endif

    QWidget::keyPressEvent(event);
}

void DeskControler::mousePressEvent(QMouseEvent *event)
{
    // ============ 调试退出功能 ============
    // 在左上角区域连续快速点击5次可退出应用，呼出设置桌面启动器的页面，设置别的桌面即可打破Kiosk模式

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
                //LogWidget::instance()->addLog(QString("调试退出: 点击 %1/%2").arg(m_debugExitTapCount).arg(DEBUG_EXIT_TAP_COUNT), LogWidget::Info);
            }
            else
            {
                m_debugExitTapCount++;
                //LogWidget::instance()->addLog(QString("调试退出: 点击 %1/%2").arg(m_debugExitTapCount).arg(DEBUG_EXIT_TAP_COUNT), LogWidget::Info);

                if (m_debugExitTapCount >= DEBUG_EXIT_TAP_COUNT)
                {
                    //LogWidget::instance()->addLog("调试退出: 触发退出!", LogWidget::Warning);

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

bool DeskControler::eventFilter(QObject *watched, QEvent *event)
{
    // 监听 TopContainer 的点击事件
    if (watched->objectName() == "TopContainer" && event->type() == QEvent::MouseButtonPress)
    {
        toggleTopMenu(); // 触发动画
        return true;     // 拦截事件
    }
    return QWidget::eventFilter(watched, event);
}
