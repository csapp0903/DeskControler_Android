#ifndef ANDROIDVIDEOSURFACE_H
#define ANDROIDVIDEOSURFACE_H

#include <QAbstractVideoSurface>

class AndroidVideoSurface : public QAbstractVideoSurface
{
    Q_OBJECT

public:
    explicit AndroidVideoSurface(QObject *parent = nullptr);
    ~AndroidVideoSurface();

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
        QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const override;

    bool present(const QVideoFrame &frame) override;

signals:
    void CameraImage(const QImage &image);
};

#endif // ANDROIDVIDEOSURFACE_H
