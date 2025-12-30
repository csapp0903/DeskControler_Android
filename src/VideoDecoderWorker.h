#ifndef VIDEODECODERWORKER_H
#define VIDEODECODERWORKER_H

#include <QObject>
#include <QByteArray>
#include <QImage>

// FFmpeg 相关头文件
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <QQueue>
#include <QMutex>

class VideoDecoderWorker : public QObject
{
    Q_OBJECT

public:
    explicit VideoDecoderWorker(QObject* parent = nullptr);
    ~VideoDecoderWorker();
    void cleanup();

public slots:
    void decodePacket(const QByteArray& packetData);
    void decodePacket1(const QByteArray& packetData);

signals:
    void frameDecoded(const QImage& image);

private:
    void processQueue();

    const AVCodec* codec = nullptr;
    AVCodecContext* codecCtx = nullptr;
    AVFrame* frame = nullptr;
    SwsContext* swsCtx = nullptr;

    mutable QMutex m_mutex;
    QQueue<QByteArray> m_queue;
};

#endif // VIDEODECODERWORKER_H
