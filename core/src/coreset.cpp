#include "coreset.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>

CORE_NAMESPACE_S
const QString coreSet::settingFileName  = "doodle_conf.json";
coreSet::coreSet()
{

}

void coreSet::getSetting()
{
    if(doc.exists(settingFileName)){
        QFile strFile(doc.absoluteFilePath(settingFileName));
        if(!strFile.open(QIODevice::ReadOnly)) throw std::runtime_error("not open doodle_conf.json");
        QJsonDocument jsondoc = QJsonDocument::fromBinaryData(strFile.readAll());
        QJsonObject jsonObj = jsondoc.object();
    }

}
CORE_DNAMESPACE_E
