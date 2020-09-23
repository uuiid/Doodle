#pragma once

#include "core_global.h"
#include "coresqldata.h"

#include <QVector>
#include <QFileInfo>
#include <QSharedPointer>
#include <QWeakPointer>


CORE_NAMESPACE_S

typedef QVector<QFileInfo> QfileListPtr;

class CORE_EXPORT fileSqlInfo :public coresqldata
{
public:
    //属性设置和查询
    fileSqlInfo();
    QfileListPtr getFileList() const;
    virtual void setFileList(const QfileListPtr &filelist);

    int getVersionP() const;
    void setVersionP(const int &value);

    QString getInfoP() const;
    void setInfoP(const QString &value);

    QString getFileStateP() const;
    void setFileStateP(const QString &value);


protected:
    //属性包装
    QString fileP;
    QString fileSuffixesP;
    QString userP;
    int versionP;
    QString filepathP;
    QString infoP;
    QString fileStateP;
};


CORE_DNAMESPACE_E

