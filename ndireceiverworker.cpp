#include "ndireceiverworker.h"

#include "main.h"

#include <QPainter>
#include <QDateTime>
#include <QThread>

NdiReceiverWorker::NdiReceiverWorker(QObject* parent)
    : QObject(parent)
{
    init();
}

NdiReceiverWorker::~NdiReceiverWorker()
{
    qDebug() << "+ ~NdiReceiverWorker()";

    m_bStop = true;

    // TODO: Is there a better way to wait for `process()` to exit?
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    qDebug() << "- ~NdiReceiverWorker()";
}

void NdiReceiverWorker::init()
{
    m_bStop = false;
    m_cNdiSourceName.clear();
    m_nAudioLevelLeft = AUDIO_LEVEL_MIN;
    m_nAudioLevelRight = AUDIO_LEVEL_MIN;
    m_bMuteAudio = false;
    m_cIDX.clear();
    std::fill(std::begin(m_fAudioLevels), std::end(m_fAudioLevels), 0.0);
}

void NdiReceiverWorker::addVideoSink(QVideoSink *videoSink)
{
    if (videoSink && !m_videoSinks.contains(videoSink))
    {
        m_videoSinks.append(videoSink);
    }
}

void NdiReceiverWorker::removeVideoSink(QVideoSink *videoSink)
{
    m_videoSinks.removeAll(videoSink);
}

void NdiReceiverWorker::setNdiSourceName(QString cNdiSourceName)
{
    m_cNdiSourceName = cNdiSourceName;
}

void NdiReceiverWorker::muteAudio(bool bMute)
{
    m_bMuteAudio = bMute;
}

void NdiReceiverWorker::setAudioLevelLeft(int level)
{
    m_nAudioLevelLeft = level;
    Q_EMIT audioLevelLeftChanged(m_nAudioLevelLeft);
}

void NdiReceiverWorker::setAudioLevelRight(int level)
{
    m_nAudioLevelRight = level;
    Q_EMIT audioLevelRightChanged(m_nAudioLevelRight);
}

void NdiReceiverWorker::stop()
{
    qDebug() << "+stop()";
    m_bStop = true;
    qDebug() << "-stop()";
}

void NdiReceiverWorker::process()
{
    qDebug() << "+process()";

    QAudioSink *pAudioOutputSink = nullptr;
    QIODevice *pAudioIoDevice = nullptr;

    QAudioFormat audioFormat;
    audioFormat.setSampleRate(44100);
    audioFormat.setChannelCount(2);
    audioFormat.setSampleFormat(QAudioFormat::Float);
    //format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    qDebug() << "audioFormat" << audioFormat;

    auto audioOutputDevice = QMediaDevices::defaultAudioOutput();
    if (audioOutputDevice.isFormatSupported(audioFormat))
    {
        pAudioOutputSink = new QAudioSink(audioOutputDevice, audioFormat, this);
        pAudioOutputSink->setVolume(1.0);
        pAudioIoDevice = pAudioOutputSink->start();
    }
    else
    {
        qWarning() << "process: Requested audio format is not supported by the default audio device.";
    }

    QString                     cNdiSourceName;
    int64_t                     nVTimestamp = 0;
    int64_t                     nATimestamp = 0;
    NDIlib_recv_instance_t      pNdiRecv = nullptr;
    NDIlib_framesync_instance_t pNdiFrameSync = nullptr;

    NDIlib_video_frame_v2_t video_frame;
    NDIlib_audio_frame_v2_t audio_frame;
    NDIlib_metadata_frame_t metadata_frame;
    NDIlib_audio_frame_interleaved_32f_t a32f;
    size_t nAudioBufferSize = 0;

    while (!m_bStop)
    {
        /*
        // Is anything selected
        if (m_cNdiSourceName.isEmpty() || m_cNdiSourceName.isNull())
        {
            qDebug() << "No NDI source selected; continue";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        */

        if (cNdiSourceName != m_cNdiSourceName)
        {
            // Change source

            //cfg->setRecStatus(0);

            // TODO: emit source changing...
            emit newVideoFrame();
            setAudioLevelLeft(AUDIO_LEVEL_MIN);
            setAudioLevelRight(AUDIO_LEVEL_MIN);

            auto _ndiSources = ndi().ndiFindSources();
            auto ndiList = _ndiSources.values();
            if (!ndiList.size())
            {
                if (true)
                {
                    qDebug() << "No NDI sources; continue";
                }
                // TODO: Tune this duration?
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            int nSourceChannel = -1;
            for (int i = 0; i < _ndiSources.size(); i++)
            {
                auto _cNdiSourceName = QString::fromUtf8(ndiList.at(i).p_ndi_name);
                if (QString::compare(_cNdiSourceName, m_cNdiSourceName, Qt::CaseInsensitive) == 0)
                {
                    nSourceChannel = i;
                    cNdiSourceName = _cNdiSourceName;
                    // TODO: Why sleep?
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    break; // for _ndiSources
                }
                else
                {
                    // TODO: Why sleep?
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue; // while
                }
            }
            if (nSourceChannel == -1)
            {
                // TODO: Why sleep?
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            if (true)
            {
                qDebug() << "NDI source activated: " << cNdiSourceName;
            }

            if (true)
            {
                qDebug() << "NDI init starts...";
            }

            if (pNdiRecv != nullptr)
            {
                if (pAudioOutputSink && !m_bStop)
                {
                    if (true)
                    {
                        qDebug() << "NDI audio stop.";
                    }
                    pAudioOutputSink->stop();
                    pAudioIoDevice = nullptr;
                }

                NDIlib_recv_destroy(pNdiRecv);
                pNdiRecv = nullptr;
            }

            NDIlib_recv_create_v3_t recv_desc;
            recv_desc.source_to_connect_to = ndiList.at(nSourceChannel);
            //recv_desc.color_format = NDIlib_recv_color_format_UYVY_BGRA;
            recv_desc.color_format = NDIlib_recv_color_format_best;
            recv_desc.bandwidth = NDIlib_recv_bandwidth_highest;
            recv_desc.allow_video_fields = true;
            if (true)
            {
                qDebug() << "NDI_recv_create_desc.source_to_connect_to=" << recv_desc.source_to_connect_to.p_ndi_name;
                qDebug() << "NDI_recv_create_desc.color_format=" << recv_desc.color_format;
                qDebug() << "NDI_recv_create_desc.bandwidth=" << recv_desc.bandwidth;
            }

            qDebug() << "NDIlib_recv_create_v3";
            pNdiRecv = NDIlib_recv_create_v3(&recv_desc);
            qDebug() << "m_pNdiRecv=" << pNdiRecv;
            if (pNdiRecv)
            {
                nVTimestamp = 0;
                nATimestamp = 0;

                if (pNdiFrameSync != nullptr)
                {
                    NDIlib_framesync_destroy(pNdiFrameSync);
                }
                qDebug() << "NDIlib_framesync_create";
                pNdiFrameSync = NDIlib_framesync_create(pNdiRecv);
                Q_ASSERT(pNdiFrameSync != nullptr);

                NDIlib_metadata_frame_t enable_hw_accel;
                enable_hw_accel.p_data = (char*)"<ndi_video_codec type=\"hardware\"/>";
                qDebug() << "NDIlib_recv_send_metadata" << enable_hw_accel.p_data;
                NDIlib_recv_send_metadata(pNdiRecv, &enable_hw_accel);

                //cfg->setRecStatus(1);
            }

            if (pAudioOutputSink && !m_bStop)
            {
                if (pAudioIoDevice == nullptr)
                {
                    pAudioIoDevice = pAudioOutputSink->start();
                    if (pAudioIoDevice && true)
                    {
                        qDebug() << "NDI receiver audio started.";
                    }
                }
            }
        }

        if (cNdiSourceName.isEmpty() || cNdiSourceName.isNull())
        {
            qDebug() << "No NDI source selected; continue";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        else
        {
            //if ()
        }

        //cNdiSourceName = m_cNdiSourceName;

        //VIDEO
        // TODO: Consider using this once I: 1) maybe need to pass the queue, 2) learn how to pass a signal
        //ndi().ndiReceiveVideo(pNdiFrameSync, &video_frame, &nVTimestamp);
        // until then...
        video_frame = {};
        //qDebug() << "NDIlib_framesync_capture_video";
        NDIlib_framesync_capture_video(pNdiFrameSync, &video_frame, NDIlib_frame_format_type_progressive);
        //qDebug() << "video_frame.p_data=" << video_frame.p_data;
        // if video_frame.p_data is all zeros the save that as a bitmap?
        if (video_frame.p_data && (video_frame.timestamp > nVTimestamp))
        {
            //qDebug() << "video_frame";
            nVTimestamp = video_frame.timestamp;
            processVideo(&video_frame, m_videoSinks);
        }
        NDIlib_framesync_free_video(pNdiFrameSync, &video_frame);

        //AUDIO
        audio_frame = {};
        NDIlib_framesync_capture_audio(pNdiFrameSync, &audio_frame, audioFormat.sampleRate(), audioFormat.channelCount(), 1024);
        if (audio_frame.p_data && (audio_frame.timestamp > nATimestamp))
        {
            nATimestamp = audio_frame.timestamp;
            processAudio(&audio_frame, &a32f, &nAudioBufferSize, pAudioIoDevice);
        }
        NDIlib_framesync_free_audio(pNdiFrameSync, &audio_frame);

        auto frameType = NDIlib_recv_capture_v2(pNdiRecv, nullptr, nullptr, &metadata_frame, 0);
        switch(frameType)
        {
        case NDIlib_frame_type_e::NDIlib_frame_type_metadata:
            qDebug() << "NDIlib_frame_type_metadata";
            processMetaData(&metadata_frame);
            break;
        case NDIlib_frame_type_e::NDIlib_frame_type_status_change:
            qDebug() << "NDIlib_frame_type_status_change";
            break;
        default:
            // ignore
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (a32f.p_data)
    {
        delete[] a32f.p_data;
        a32f.p_data = nullptr;
    }

    if (pNdiFrameSync)
    {
        NDIlib_framesync_destroy(pNdiFrameSync);
        pNdiFrameSync = nullptr;
    }

    if (pNdiRecv)
    {
        NDIlib_recv_destroy(pNdiRecv);
        pNdiRecv = nullptr;
    }

    if (pAudioOutputSink)
    {
        pAudioOutputSink->stop();
        delete pAudioOutputSink;
        pAudioOutputSink = nullptr;
        pAudioIoDevice = nullptr;
    }

    qDebug() << "-process()";
}

void NdiReceiverWorker::processMetaData(NDIlib_metadata_frame_t *pNdiMetadataFrame)
{
    QString metadata = QString::fromUtf8(pNdiMetadataFrame->p_data, pNdiMetadataFrame->length);
    qDebug() << "metadata" << metadata;
}

//#define MY_VERBOSE_LOGGING

QString withCommas(int value)
{
    return QLocale(QLocale::English).toString(value);
}

void NdiReceiverWorker::processVideo(NDIlib_video_frame_v2_t *pNdiVideoFrame, QList<QVideoSink*> videoSinks)
{
    auto ndiWidth = pNdiVideoFrame->xres;
    auto ndiHeight = pNdiVideoFrame->yres;
    auto ndiLineStrideInBytes = pNdiVideoFrame->line_stride_in_bytes;
    auto ndiPixelFormat = pNdiVideoFrame->FourCC;
#if defined(MY_VERBOSE_LOGGING)
    qDebug();
    qDebug() << "+processVideo(...)";
    auto ndiArea = ndiWidth * ndiHeight;
    auto ndiFrameType = pNdiVideoFrame->frame_format_type;
    qDebug().noquote() << "pNdiVideoFrame Size (X x Y = Pixels):" << withCommas(ndiWidth) << "x" << withCommas(ndiHeight) << "=" << withCommas(ndiArea);
    qDebug() << "pNdiVideoFrame->line_stride_in_bytes" << withCommas(ndiLineStrideInBytes);
    qDebug() << "pNdiVideoFrame->frame_format_type" << ndiFrameTypeToString(ndiFrameType);
    qDebug() << "pNdiVideoFrame->FourCC:" << ndiFourCCToString(ndiPixelFormat);
#endif
    auto pixelFormat = ndiPixelFormatToPixelFormat(ndiPixelFormat);
#if defined(MY_VERBOSE_LOGGING) && false
    qDebug() << "pixelFormat" << QVideoFrameFormat::pixelFormatToString(pixelFormat);
#endif
    if (pixelFormat == QVideoFrameFormat::PixelFormat::Format_Invalid)
    {
        qDebug().nospace() << "Unsupported pNdiVideoFrame->FourCC " << ndiFourCCToString(ndiPixelFormat) << "; return;";
        return;
    }

    QSize videoFrameSize(ndiWidth, ndiHeight);
    QVideoFrameFormat videoFrameFormat(videoFrameSize, pixelFormat);
    //videoFrameFormat.setYCbCrColorSpace(QVideoFrameFormat::YCbCrColorSpace::?); // Does this make any difference?
    QVideoFrame videoFrame(videoFrameFormat);

    if (!videoFrame.map(QVideoFrame::WriteOnly))
    {
        qWarning() << "videoFrame.map(QVideoFrame::WriteOnly) failed; return;";
        return;
    }

    auto pDstY = videoFrame.bits(0);
    auto pSrcY = pNdiVideoFrame->p_data;
    auto pDstUV = videoFrame.bits(1);
    auto pSrcUV = pSrcY + (ndiLineStrideInBytes * ndiHeight);
    for (int line = 0; line < ndiHeight; ++line)
    {
        memcpy(pDstY, pSrcY, ndiLineStrideInBytes);
        pDstY += ndiLineStrideInBytes;
        pSrcY += ndiLineStrideInBytes;
        // For now QVideoFrameFormat/QVideoFrame does not support P216. :(
        // I have started the conversation to have it added, but that may take awhile. :(
        // Until then, a cheap way to downsample P216's 4:2:2 to P016's 4:2:0 chroma sampling
        // is to just scan every other UV line.
        // There are still a few visible artifacts on the screen, but it is passable.
        if (line % 2)
        {
            memcpy(pDstUV, pSrcUV, ndiLineStrideInBytes);
            pDstUV += ndiLineStrideInBytes;
        }
        pSrcUV += ndiLineStrideInBytes;
    }

#if false
    // TODO: Find way to overlay FPS counter
    QImage::Format image_format = QVideoFrameFormat::imageFormatFromPixelFormat(outff.pixelFormat());
    QImage image(outff.bits(0), video_frame->xres, video_frame->yres, image_format);
    image.fill(Qt::green);

    QPainter painter(&image);
    painter.drawText(image.rect(), Qt::AlignCenter, QDateTime::currentDateTime().toString());
    painter.end();
#endif

    videoFrame.unmap();

    foreach(QVideoSink *videoSink, videoSinks)
    {
        videoSink->setVideoFrame(videoFrame);
    }

#if defined(MY_VERBOSE_LOGGING)
    qDebug() << "-processVideo(...)";
#endif
}

QString NdiReceiverWorker::ndiFrameTypeToString(NDIlib_frame_format_type_e ndiFrameType)
{
    QString s;
    switch(ndiFrameType)
    {
    case NDIlib_frame_format_type_progressive:
        s = "NDIlib_frame_format_type_progressive";
        break;
    case NDIlib_frame_format_type_interleaved:
        s = "NDIlib_frame_format_type_interleaved";
        break;
    case NDIlib_frame_format_type_field_0:
        s = "NDIlib_frame_format_type_field_0";
        break;
    case NDIlib_frame_format_type_field_1:
        s = "NDIlib_frame_format_type_field_1";
        break;
    default:
        s = "UNKNOWN";
        break;
    }
    return s + s.asprintf(" (0x%X)", ndiFrameType);
}

/**
 * Inverse of the NDI_LIB_FOURCC macro defined in `Processing.NDI.structs.h`
 */
QString NdiReceiverWorker::ndiFourCCToString(NDIlib_FourCC_video_type_e ndiFourCC)
{
    QString s;
    s += (char)(ndiFourCC >> 0 & 0x000000FF);
    s += (char)(ndiFourCC >> 8 & 0x000000FF);
    s += (char)(ndiFourCC >> 16 & 0x000000FF);
    s += (char)(ndiFourCC >> 24 & 0x000000FF);
    return s + s.asprintf(" (0x%08X)", ndiFourCC);
}

QVideoFrameFormat::PixelFormat NdiReceiverWorker::ndiPixelFormatToPixelFormat(enum NDIlib_FourCC_video_type_e ndiFourCC)
{
    switch(ndiFourCC)
    {
    case NDIlib_FourCC_video_type_UYVY:
        return QVideoFrameFormat::PixelFormat::Format_UYVY;
    case NDIlib_FourCC_video_type_UYVA:
        return QVideoFrameFormat::PixelFormat::Format_UYVY;
        break;
    // Result when requesting NDIlib_recv_color_format_best
    case NDIlib_FourCC_video_type_P216:
        return QVideoFrameFormat::PixelFormat::Format_P016;
    //case NDIlib_FourCC_video_type_PA16:
    //    return QVideoFrameFormat::PixelFormat::?;
    case NDIlib_FourCC_video_type_YV12:
        return QVideoFrameFormat::PixelFormat::Format_YV12;
    //case NDIlib_FourCC_video_type_I420:
    //    return QVideoFrameFormat::PixelFormat::?
    case NDIlib_FourCC_video_type_NV12:
        return QVideoFrameFormat::PixelFormat::Format_NV12;
    case NDIlib_FourCC_video_type_BGRA:
        return QVideoFrameFormat::PixelFormat::Format_BGRA8888;
    case NDIlib_FourCC_video_type_BGRX:
        return QVideoFrameFormat::PixelFormat::Format_BGRX8888;
    case NDIlib_FourCC_video_type_RGBA:
        return QVideoFrameFormat::PixelFormat::Format_RGBA8888;
    case NDIlib_FourCC_video_type_RGBX:
        return QVideoFrameFormat::PixelFormat::Format_RGBX8888;
    default:
        return QVideoFrameFormat::PixelFormat::Format_Invalid;
    }
}

void NdiReceiverWorker::processAudio(NDIlib_audio_frame_v2_t *pNdiAudioFrame, NDIlib_audio_frame_interleaved_32f_t *pA32f, size_t *pnAudioBufferSize, QIODevice *pAudioIoDevice)
{
#if defined(MY_VERBOSE_LOGGING)
    qDebug();
    qDebug() << "+processAudio(...)";
#endif
    if (m_bMuteAudio)
    {
        qDebug() << "m_bMuteAudio == true; return;";
        return;
    }
    if (pAudioIoDevice == nullptr)
    {
        qDebug() << "pAudioIoDevice == nullptr; return;";
        return;
    }

    size_t nThisAudioBufferSize = pNdiAudioFrame->no_samples * pNdiAudioFrame->no_channels * sizeof(float);
#if defined(MY_VERBOSE_LOGGING)
    qDebug() << "nThisAudioBufferSize" << nThisAudioBufferSize;
#endif
    if (nThisAudioBufferSize > *pnAudioBufferSize)
    {
        qDebug() << "growing pA32f->p_data to " << nThisAudioBufferSize << "bytes";
        if (pA32f->p_data)
        {
            delete[] pA32f->p_data;
        }
        pA32f->p_data = new float[nThisAudioBufferSize];
        *pnAudioBufferSize = nThisAudioBufferSize;
    }

    NDIlib_util_audio_to_interleaved_32f_v2(pNdiAudioFrame, pA32f);

    size_t nWritten = 0;
    do
    {
        nWritten += pAudioIoDevice->write((const char*)pA32f->p_data + nWritten, nThisAudioBufferSize);
#if defined(MY_VERBOSE_LOGGING)
        qDebug() << "nWritten" << nWritten;
#endif
    } while (nWritten < nThisAudioBufferSize);
#if defined(MY_VERBOSE_LOGGING)
    qDebug() << "-processAudio(...)";
#endif
}
