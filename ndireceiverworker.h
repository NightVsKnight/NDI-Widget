#ifndef NDIRECEIVERWORKER_H
#define NDIRECEIVERWORKER_H

#include <QObject>
#include <QVideoSink>
#include <QVideoFrame>

#include <QMediaDevices>
#include <QAudioFormat>
#include <QAudioSink>

#include "main.h"

#define MAX_AUDIO_LEVELS 16
#define AUDIO_LEVEL_MIN -60

class NdiReceiverWorker : public QObject
{
    Q_OBJECT
public:
    NdiReceiverWorker(QObject *parent = nullptr);
   ~NdiReceiverWorker();

    void addVideoSink(QVideoSink *videoSink);
    void removeVideoSink(QVideoSink *videoSink);
    void setNdiSourceName(QString cNdiSourceName);
    void muteAudio (bool bMute);

Q_SIGNALS:
    void newVideoFrame();
    void audioLevelLeftChanged(int);
    void audioLevelRightChanged(int);

public slots:
    void stop();
    void process();

private:
    QList<QVideoSink*> m_videoSinks;

    bool          m_bStop;
    QString       m_cNdiSourceName;
    int           m_nAudioLevelLeft;
    int           m_nAudioLevelRight;
    volatile bool m_bMuteAudio;
    volatile bool m_bLowQuality;
    QString       m_cIDX;

    float m_fAudioLevels[MAX_AUDIO_LEVELS];

    void init();
    void setAudioLevelLeft(int level);
    void setAudioLevelRight(int level);
    void processVideo(NDIlib_video_frame_v2_t *pNdiVideoFrame, QList<QVideoSink*> videoSinks);
    void processAudio(NDIlib_audio_frame_v2_t *pNdiAudioFrame, NDIlib_audio_frame_interleaved_32f_t *pA32f, size_t *pnAudioBufferSize, QIODevice *pAudioIoDevice);
    void processMetaData(NDIlib_metadata_frame_t *pNdiMetadataFrame);
    static QString ndiFrameTypeToString(NDIlib_frame_format_type_e ndiFrameType);
    static QString ndiFourCCToString(NDIlib_FourCC_video_type_e ndiFourCC);
    static QVideoFrameFormat::PixelFormat ndiPixelFormatToPixelFormat(enum NDIlib_FourCC_video_type_e ndiFourCC);
};

#endif // NDIRECEIVERWORKER_H
