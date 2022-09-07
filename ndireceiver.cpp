#include "ndireceiver.h"
#include "ndireceiverworker.h"

class NdiReceiverData : public QSharedData
{
public:
};

NdiReceiver::NdiReceiver(QObject *parent)
    : QObject(parent),
      data(new NdiReceiverData)
{
    init();
}

NdiReceiver::NdiReceiver(const NdiReceiver &rhs) : data(rhs.data)
{
    init();
}

NdiReceiver &NdiReceiver::operator=(const NdiReceiver &rhs)
{
    if (this != &rhs) data.operator=(rhs.data);
    return *this;
}

NdiReceiver::~NdiReceiver()
{
    stop();
}

void NdiReceiver::init()
{
    workerNdiReceiver.moveToThread(&threadNdiReceiver);
    connect(&threadNdiReceiver, &QThread::started, &workerNdiReceiver, &NdiReceiverWorker::process);
    connect(&threadNdiReceiver, &QThread::finished, &workerNdiReceiver, &NdiReceiverWorker::stop);
}

void NdiReceiver::addVideoSink(QVideoSink *videoSink) {
    workerNdiReceiver.addVideoSink(videoSink);
}

void NdiReceiver::setNdiSourceName(QString ndiSourceName)
{
    workerNdiReceiver.setNdiSourceName(ndiSourceName);
}

void NdiReceiver::start()
{
    threadNdiReceiver.start();
}

void NdiReceiver::stop()
{
    workerNdiReceiver.stop();
    threadNdiReceiver.quit();
    threadNdiReceiver.wait();
}
