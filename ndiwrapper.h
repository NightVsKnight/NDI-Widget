#ifndef CNDIWRAPPER_H
#define CNDIWRAPPER_H

#include <QObject>
#include <QSharedDataPointer>
#include <QMap>

#include <Processing.NDI.Lib.h>

class CNdiWrapperData;

class CNdiWrapper : public QObject
{
    Q_OBJECT
public:
    explicit CNdiWrapper(QObject *parent = nullptr);
    CNdiWrapper(const CNdiWrapper &);
    CNdiWrapper &operator=(const CNdiWrapper &);
    ~CNdiWrapper();

    bool isNdiInitialized();
    bool ndiInitialize();
    void ndiDestroy();
    QMap<QString, NDIlib_source_t> ndiFindSources();

signals:
    //...

private:
    QSharedDataPointer<CNdiWrapperData> data;

    NDIlib_find_instance_t m_pNDI_find = nullptr;
};

#endif // CNDIWRAPPER_H
