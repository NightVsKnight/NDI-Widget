#ifndef NDIRECEIVER_H
#define NDIRECEIVER_H

#include <QSharedDataPointer>
#include <QThread>

#include "ndireceiverworker.h"

class NdiReceiverData;

class NdiReceiver : public QObject
{
    Q_OBJECT
public:
    explicit NdiReceiver(QObject *parent = nullptr);
    NdiReceiver(const NdiReceiver &);
    NdiReceiver &operator=(const NdiReceiver &);
    ~NdiReceiver();

    void addVideoSink(QVideoSink *videoSink);
    void setNdiSourceName(QString ndiSourceName);
    void start();
    void stop();

private:
    QSharedDataPointer<NdiReceiverData> data;

    NdiReceiverWorker workerNdiReceiver;
    QThread threadNdiReceiver;

    void init();
};

#endif // NDIRECEIVER_H
