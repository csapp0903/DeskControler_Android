#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QImage>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPushButton>

class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget* parent = nullptr);

signals:
    void mouseEventCaptured(int x, int y, int mask, int value);
    void touchEventCaptured(QVariant value);
    void keyEventCaptured(int key, bool pressed);

    void closeBtnClicked();

public slots:
    void setFrame(const QImage& image);

    void setPreValue(const qreal &scale);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    bool event(QEvent *event) override;

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    QImage m_currentFrame;
    bool m_firstFrame = true;
    qreal m_scale = 1.0;
    QPoint m_hoverPt = QPoint(0, 0);

    void handleMouseEvent(QPointF pos, int mask, int value);
    bool handleTouchEvent(QTouchEvent* event);

    QPushButton *m_closeBtn = nullptr;
};

#endif // VIDEOWIDGET_H
