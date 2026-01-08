#include "VideoWidget.h"
#include <QPainter>
#include <QKeyEvent>
#include <QDebug>

#include "LogWidget.h"
#include "DeskDefine.h"

#define BTN_FIXED_W 80

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent)
    , m_offsetX(0)
    , m_offsetY(0)
{
    //setWindowFlags(Qt::Window);
    setFocusPolicy(Qt::StrongFocus);
    // 启用鼠标跟踪以接收鼠标移动事件
    setMouseTracking(true);
    // 启用触摸事件
    setAttribute(Qt::WA_AcceptTouchEvents);
    // 禁用单点触摸优化
    setAttribute(Qt::WA_TouchPadAcceptSingleTouchEvents, false);
    // 启用悬停事件
    setAttribute(Qt::WA_Hover);

    setAttribute(Qt::WA_OpaquePaintEvent, false);

    m_closeBtn = new QPushButton("X", this);
    m_closeBtn->setFixedSize(BTN_FIXED_W, BTN_FIXED_W);
    connect(m_closeBtn, &QPushButton::released, this, &VideoWidget::closeBtnClicked);

    m_closeBtn->hide();
}

void VideoWidget::setFrame(const QImage& image)
{
    // LogWidget::instance()->addLog(QString("[VideoWidget] setFrame, size: %1x%2, isNull: %3")
    //                                   .arg(image.width()).arg(image.height()).arg(image.isNull()), LogWidget::Info);

    m_currentFrame = image;

    // // 首帧时设置固定尺寸（用于滚动区域）
    // if (m_firstFrame && !m_currentFrame.isNull())
    // {
    //     setMinimumSize(m_currentFrame.size());
    //     resize(m_currentFrame.size());
    //     m_firstFrame = false;
    // }

    update();
}

void VideoWidget::setPreValue(const qreal &scale)
{
    m_scale = scale;
}

// void VideoWidget::paintEvent(QPaintEvent* event)
// {
//     Q_UNUSED(event);
//     QPainter painter(this);
//     if (!m_currentFrame.isNull())
//     {
//         painter.drawImage(rect(), m_currentFrame);
//     }
//     else
//     {
//         painter.fillRect(rect(), Qt::black);
//     }
// }
void VideoWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    painter.fillRect(rect(), Qt::black);

    if (m_currentFrame.isNull())
    {
        return;
    }

    // 计算保持比例的缩放
    QSize widgetSize = this->size();
    QSize imageSize = m_currentFrame.size();

    if (imageSize.isEmpty()) return;

    qreal scaleW = (qreal)widgetSize.width() / imageSize.width();
    qreal scaleH = (qreal)widgetSize.height() / imageSize.height();

    m_scale = qMin(scaleW, scaleH);

    // 计算实际显示的宽高
    int drawW = static_cast<int>(imageSize.width() * m_scale);
    int drawH = static_cast<int>(imageSize.height() * m_scale);

    // 计算偏移量
    // 水平居中
    m_offsetX = (widgetSize.width() - drawW) / 2;

    // 垂直下对齐
    // 如果要垂直居中，则是 (widgetSize.height() - drawH) / 2
    m_offsetY = widgetSize.height() - drawH;

    // 在计算出的区域绘制图像
    QRect drawRect(m_offsetX, m_offsetY, drawW, drawH);
    painter.drawImage(drawRect, m_currentFrame);
}

void VideoWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->source() != Qt::MouseEventNotSynthesized)
    {
        return;
    }
    qDebug() << "-----mousePressEvent" << (event->source() == Qt::MouseEventNotSynthesized);

    int mask = 0;
    if (event->button() == Qt::LeftButton)
        mask = MouseLeftDown;
    else if (event->button() == Qt::RightButton)
        mask = MouseRightClick;
    else if (event->button() == Qt::MiddleButton)
        mask = MouseMiddleClick;

    handleMouseEvent(event->pos(), mask, 0);
}

void VideoWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->source() != Qt::MouseEventNotSynthesized)
    {
        return;
    }
    qDebug() << "-----mouseReleaseEvent" << (event->source() == Qt::MouseEventNotSynthesized);

    int mask = 0;
    if (event->button() == Qt::LeftButton)
    {
        mask = MouseLeftUp;
    }

    handleMouseEvent(event->pos(), mask, 0);
}

bool VideoWidget::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::Wheel:
    {
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);

        // 获取滚轮滚动的角度
        int angleDelta = wheelEvent->angleDelta().y();

        QPoint pos = wheelEvent->position().toPoint();
        //qDebug() << "--------QWheelEvent angleDelta:" << angleDelta << pos;

        handleMouseEvent(pos, MouseWheel, angleDelta);
        break;
    }
    case QEvent::HoverMove:
    {
        QHoverEvent* hoverEvent = static_cast<QHoverEvent*>(event);

        // 只有变动时才发送 HoverMove
        QPoint pos = hoverEvent->pos();
        if (m_hoverPt != pos)
        {
            m_hoverPt = pos;

            handleMouseEvent(pos, MouseMove, 0);
        }
        //qDebug() << "--------QHoverEvent:" << hoverEvent->pos();
        break;
    }
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    {
        return handleTouchEvent(static_cast<QTouchEvent*>(event));
    }
    case QEvent::Resize:
    {
        m_closeBtn->move(rect().right() - m_closeBtn->width(), 0);
    }
    default:
        break;
    }

    return QWidget::event(event);
}

void VideoWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    handleMouseEvent(event->pos(), MouseDoubleClick, 0);
}

void VideoWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (event->source() != Qt::MouseEventNotSynthesized)
    {
        return;
    }
    //qDebug() << "-----mouseMoveEvent" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << (event->source() == Qt::MouseEventNotSynthesized);

    if (event->buttons() & Qt::LeftButton)
    {
        QPointF pos = event->pos();
        handleMouseEvent(pos, MouseMove, 0);
    }
}

void VideoWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) return;
    LogWidget::instance()->addLog("keyPressEvent ", LogWidget::Warning);
    emit keyEventCaptured(event->key(), true);
}

void VideoWidget::keyReleaseEvent(QKeyEvent* event)
{
    int key = event->key();
    QString keyStr = QKeySequence(key).toString();
    //qDebug() << "-----keyReleaseStr:" << key << keyStr;

    LogWidget::instance()->addLog("keyReleaseEvent ", LogWidget::Warning);
    if (event->isAutoRepeat()) return;
    emit keyEventCaptured(event->key(), false);
}

// void VideoWidget::handleMouseEvent(QPointF pos, int mask, int value)
// {
//     if (!pos.isNull())
//     {
//         emit mouseEventCaptured(static_cast<int>(pos.x() / m_scale), static_cast<int>(pos.y() / m_scale), mask, value);
//     }
// }
void VideoWidget::handleMouseEvent(QPointF pos, int mask, int value)
{
    if (!pos.isNull() && m_scale > 0)
    {
        // 先减去偏移量，再除以缩放比例
        int x = static_cast<int>((pos.x() - m_offsetX) / m_scale);
        int y = static_cast<int>((pos.y() - m_offsetY) / m_scale);

        emit mouseEventCaptured(x, y, mask, value);
    }
}

bool VideoWidget::handleTouchEvent(QTouchEvent *event)
{
    DeskTouchEvent protoEvent;
    protoEvent.timestamp = QDateTime::currentMSecsSinceEpoch();
    QList<DeskTouchPoint> points;

    QList<QTouchEvent::TouchPoint> touchPoints = event->touchPoints();

    // 判断点是不是在关闭按钮位置
    if (0)
    // if (touchPoints.size() == 1)
    {
        QPoint touchPos = touchPoints.first().pos().toPoint();
        touchPos = this->mapToGlobal(touchPos);

        QPoint aa = m_closeBtn->mapToGlobal(QPoint(0, 0));
        //qDebug() << "-----pos:" << touchPos << aa << QRect(aa,  m_closeBtn->size()) << QRect(aa,  m_closeBtn->size()).contains(touchPos);
        if (QRect(aa, m_closeBtn->size()).contains(touchPos))
        {
            //m_closeBtn->clicked();
            return false;
        }
    }

    DeskTouchPhase touchPhase = TOUCH_CANCEL;
    switch (event->type())
    {
    case QEvent::TouchBegin:
    {
        touchPhase = TOUCH_BEGIN;
        break;
    }
    case QEvent::TouchUpdate:
    {
        touchPhase = TOUCH_MOVE;
        break;
    }
    case QEvent::TouchEnd:
    {
        touchPhase = TOUCH_END;
        break;
    }
    default:
        break;
    }

    static int count = 0;
    count += 1;

    for (const auto &pt : touchPoints)
    {
        DeskTouchPoint touchPt;
        touchPt.id = pt.id();
        // touchPt.x  = pt.pos().x() / m_scale;
        // touchPt.y  = pt.pos().y() / m_scale;
        // 触摸点坐标转换
        if (m_scale > 0) {
            touchPt.x  = (pt.pos().x() - m_offsetX) / m_scale;
            touchPt.y  = (pt.pos().y() - m_offsetY) / m_scale;
        } else {
            touchPt.x = 0;
            touchPt.y = 0;
        }
        touchPt.phase = touchPhase;
        touchPt.pressure = pt.pressure();
        touchPt.size = pt.ellipseDiameters().width();

        // QString timeTemp = QDateTime::currentDateTime().toString("yyyy-hh-mm ss.zzz");
        // qDebug() << "["+timeTemp+"]" << "----touchPoints:" << touchPt << touchPhase << touchPoints.size() << count;

        points.append(touchPt);
    }

    protoEvent.points = points;

    emit touchEventCaptured(QVariant::fromValue(protoEvent));

    event->accept(); // 必须调用accept()

    return true;
}
