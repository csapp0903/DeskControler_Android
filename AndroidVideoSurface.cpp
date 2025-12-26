#include "AndroidVideoSurface.h"

#include <QDebug>

AndroidVideoSurface::AndroidVideoSurface(QObject *parent)
    : QAbstractVideoSurface{parent}
{

}

AndroidVideoSurface::~AndroidVideoSurface()
{

}

QList<QVideoFrame::PixelFormat> AndroidVideoSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
    Q_UNUSED(handleType);

    return {QVideoFrame::Format_NV21, QVideoFrame::Format_YV12, QVideoFrame::Format_RGB32};
}

bool AndroidVideoSurface::present(const QVideoFrame &frame)
{
    //qDebug() << "---present:" << frame.isValid() << frame.height() << frame.width();
    if (!frame.isValid())
    {
        return false;
    }

    // 将视频帧转换为QImage
    QVideoFrame cloneFrame(frame);
    if (!cloneFrame.map(QAbstractVideoBuffer::ReadOnly))
    {
        return false;
    }

    const uchar *bits = frame.bits();
    int width = frame.width();
    int height = frame.height();
    QImage image(width, height, QImage::Format_ARGB32);

    // 色域变换
    for (int y = 0; y < height; ++y)
    {
        const uint32_t *src = reinterpret_cast<const uint32_t*>(bits + y * frame.bytesPerLine());
        uint32_t *dst = reinterpret_cast<uint32_t*>(image.scanLine(y));

        for (int x = 0; x < width; ++x)
        {
            uint32_t pixel = src[x];
            // ABGR -> ARGB 转换
            dst[x] = (pixel & 0xFF00FF00) | ((pixel & 0x00FF0000) >> 16) | ((pixel & 0x000000FF) << 16);
        }
    }

    image = image.mirrored(false, true);
    emit CameraImage(image);

    cloneFrame.unmap();

    return true;
}
