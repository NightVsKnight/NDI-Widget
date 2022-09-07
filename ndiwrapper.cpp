#include "ndiwrapper.h"

#include <QDebug>

class CNdiWrapperData : public QSharedData
{
public:
};

CNdiWrapper::CNdiWrapper(QObject *parent) : QObject(parent), data(new CNdiWrapperData)
{
}

CNdiWrapper::CNdiWrapper(const CNdiWrapper &rhs) : data(rhs.data)
{
}

CNdiWrapper &CNdiWrapper::operator=(const CNdiWrapper &rhs)
{
    if (this != &rhs) data.operator=(rhs.data);
    return *this;
}

CNdiWrapper::~CNdiWrapper()
{
    ndiDestroy();
}

//
//
//

bool CNdiWrapper::isNdiInitialized()
{
    return m_pNDI_find != nullptr;
}

bool CNdiWrapper::ndiInitialize()
{
    if (isNdiInitialized()) return true;

    auto initialized = NDIlib_initialize();
    Q_ASSERT(initialized);

    const NDIlib_find_create_t NDI_find_create_desc = { true, NULL, NULL };
    m_pNDI_find = NDIlib_find_create_v2(&NDI_find_create_desc);
    Q_ASSERT(m_pNDI_find != nullptr);

    return isNdiInitialized();
}

void CNdiWrapper::ndiDestroy()
{
    if (!isNdiInitialized()) return;

    NDIlib_find_destroy(m_pNDI_find);
    m_pNDI_find = nullptr;

    NDIlib_destroy();
}

QMap<QString, NDIlib_source_t> CNdiWrapper::ndiFindSources()
{
    QMap<QString, NDIlib_source_t> _map;
    if (!isNdiInitialized()) goto exit;
    {
        qDebug() << "finding...";
        uint32_t num_sources = 0;
        const NDIlib_source_t* p_sources = NDIlib_find_get_current_sources(m_pNDI_find, &num_sources);
        if (num_sources) {
            qDebug() << "processing" << num_sources << "NDI sources";
            for (uint32_t i = 0; i < num_sources; i++) {
                NDIlib_source_t p_source = p_sources[i];
                QString cNdiName = QString::fromUtf8(p_source.p_ndi_name);
                //qDebug() << "processing source" << i << cNdiName;
                _map.insert(cNdiName, p_source);
            }
        }
        qDebug() << "found" << _map.size() << "NDI sources";
    }
exit:
    return _map;
}
