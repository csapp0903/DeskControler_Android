#include "VideoDecoderWorker.h"
#include "LogWidget.h"

#include <QElapsedTimer>
#include <QDebug>

#define QUEUE_IMAGE 10

VideoDecoderWorker::VideoDecoderWorker(QObject* parent)
    : QObject(parent)
{
    // FFmpeg 初始化
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec)
    {
        LogWidget::instance()->addLog(QString("H264 decoder not found"), LogWidget::Error);
        return;
    }
    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx)
    {
        LogWidget::instance()->addLog(QString("Could not allocate video codec context"), LogWidget::Error);
        return;
    }
    if (avcodec_open2(codecCtx, codec, nullptr) < 0)
    {
        LogWidget::instance()->addLog(QString("Could not open codec"), LogWidget::Error);
        return;
    }
    frame = av_frame_alloc();
    if (!frame)
    {
        LogWidget::instance()->addLog(QString("Could not allocate video frame"), LogWidget::Error);
    }

    m_timer = new QTimer(this);
    m_timer->setInterval(50);
    //m_timer.setInterval(50);
    connect(m_timer, &QTimer::timeout, this, [&](){

        QMutexLocker locker(&m_mutex);

        QElapsedTimer timer;
        timer.start();

        if (m_queue.isEmpty())
        {
            return;
        }

        QByteArray data = m_queue.dequeue();
        // LogWidget::instance()->addLog(QString("[VideoDecoderWorker] Timer dequeue, size: %1, remaining: %2")
        //                                   .arg(data.size()).arg(m_queue.size()), LogWidget::Info);


        QImage image;
        bool ret = image.loadFromData(data, "JPG");
        if (ret)
        {
            // QString str1 = QString("QImage loadFromData took: %1 ms image w h: %2 %3").arg(QString::number(timer.elapsed()),
            //                                                                                QString::number(image.width()),
            //                                                                                QString::number(image.height()));
            // LogWidget::instance()->addLog(str1, LogWidget::Info);
            // qDebug() << str1;

            emit frameDecoded(image);
        }
        else
        {
            // LogWidget::instance()->addLog(QString("[VideoDecoderWorker] Decode FAILED, size: %1")
            //                                   .arg(data.size()), LogWidget::Error);
        }
    });

    m_timer->start();
}

VideoDecoderWorker::~VideoDecoderWorker()
{
    cleanup();
}

void VideoDecoderWorker::cleanup()
{
    if (m_timer) {
        m_timer->stop();
    }

    if (swsCtx)
    {
        sws_freeContext(swsCtx);
        swsCtx = nullptr;
    }
    if (frame)
    {
        av_frame_free(&frame);
        frame = nullptr;
    }
    if (codecCtx)
    {
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }
}

void VideoDecoderWorker::decodePacket(const QByteArray &packetData)
{
    //LogWidget::instance()->addLog(QString("[VideoDecoderWorker] decodePacket, size: %1").arg(packetData.size()), LogWidget::Info);
    QElapsedTimer timer;
    timer.start();

    QMutexLocker locker(&m_mutex);
    if (m_queue.size() >= QUEUE_IMAGE)
    {
        m_queue.dequeue();
    }
    m_queue.enqueue(packetData);

    // QString str = QString("packetData size: %1 queueSize: %2").arg(QString::number(packetData.size()), QString::number(m_queue.size()));
    // qDebug() << str;
    // LogWidget::instance()->addLog(str, LogWidget::Info);

    // QImage image;
    // bool ret = image.loadFromData(packetData, "JPG");
    // if (ret)
    // {
    //     QString str1 = QString("QImage loadFromData took: %1 ms image w h: %2 %3").arg(QString::number(timer.elapsed()),
    //                                                                                    QString::number(image.width()),
    //                                                                                    QString::number(image.height()));
    //     qDebug() << str1;
    //     LogWidget::instance()->addLog(str1, LogWidget::Info);

    //     emit frameDecoded(image);
    // }
}

void VideoDecoderWorker::decodePacket1(const QByteArray& packetData)
{
    // 将 packetData 拷贝到 AVPacket
    AVPacket* pkt = av_packet_alloc();
    if (!pkt)
    {
        LogWidget::instance()->addLog(QString("Could not allocate AVPacket"), LogWidget::Warning);
        return;
    }
    pkt->data = reinterpret_cast<uint8_t*>(const_cast<char*>(packetData.data()));
    pkt->size = packetData.size();

    int ret = avcodec_send_packet(codecCtx, pkt);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        // 不是完整帧的话 这边的会报错， 暂时先去掉日志
        // LogWidget::instance()->addLog(QString("Error sending packet for decoding: %1").arg(errbuf), LogWidget::Warning);
        av_packet_free(&pkt);
        return;
    }

    // 尝试接收所有解码出的帧
    while (true)
    {
        ret = avcodec_receive_frame(codecCtx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            break;
        }
        else if (ret < 0)
        {
            LogWidget::instance()->addLog(QString("Error during decoding"), LogWidget::Warning);
            break;
        }
        // 得到解码帧（YUV420P）

        // 初始化转换上下文（如果还没创建）
        if (!swsCtx)
        {
            swsCtx = sws_getContext(frame->width, frame->height, codecCtx->pix_fmt,
                                    frame->width, frame->height, AV_PIX_FMT_RGBA,
                                    SWS_BILINEAR, nullptr, nullptr, nullptr);
        }

        QImage image(frame->width, frame->height, QImage::Format_RGBA8888);
        //qDebug() << "-----image bytesPerLine:" << (image.bytesPerLine() == frame->width * 4) << codecCtx->pix_fmt;
        if (image.bytesPerLine() == frame->width * 4)
        {
            uint8_t *destData[4] = { image.bits(), nullptr, nullptr, nullptr };   // 数据指针数组
            int destLinesize[4] = { static_cast<int>(image.bytesPerLine()), 0, 0, 0 }; // 行跨度

            sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height,
                      destData, destLinesize);
        }
        else
        {
            image.fill(0);
        }

        // // 计算目标缓冲区大小
        // static int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, frame->width, frame->height, 1);
        // QByteArray imgBuffer(numBytes, 0);

        // uint8_t* destData[4] = {
        //     reinterpret_cast<uint8_t*>(imgBuffer.data()),
        //     nullptr, nullptr, nullptr
        // };
        // int destLinesize[4] = { 4 * frame->width, 0, 0, 0 };

        // // 转换为 RGBA
        // sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height,
        //           destData, destLinesize);

        // // 构造 QImage
        // QImage image(reinterpret_cast<const uchar*>(imgBuffer.constData()),
        //              frame->width, frame->height, QImage::Format_RGBA8888);
        // // 复制一份，防止堆外数据被复用
        // QImage finalImage = image.copy();

        //qDebug() << "-----VideoDecoderWorker Decoder QImage Took" << timer.elapsed() << " ms";
        // LogWidget::instance()->addLog(QString("VideoDecoderWorker Decoder QImage Took %1 ms").arg(timer.elapsed()), LogWidget::Info);

        // 发射信号，通知外部有帧已解码
        emit frameDecoded(image);
    }

    av_packet_free(&pkt);
}
