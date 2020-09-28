#pragma once

#include "core_global.h"
#include "coresqldata.h"

#include <QVector>
#include <QFileInfo>
#include <QSharedPointer>
#include <QWeakPointer>

CORE_NAMESPACE_S

class CORE_EXPORT fileSqlInfo : public coresqldata
{
public:
    //属性设置和查询
    fileSqlInfo();
    QfileInfoVector getFileList() const;
    virtual void setFileList(const QfileInfoVector &filelist);

    int getVersionP() const;
    void setVersionP(const int &value);

    QString getInfoP() const;
    void setInfoP(const QString &value);

    QString getFileStateP() const;
    void setFileStateP(const QString &value);

    virtual QString generatePath(const QString &programFodler) = 0;
    virtual QString generatePath(const QString &programFolder, const QString &suffixes) = 0;
    virtual QString generatePath(const QString &programFolder, const QString &suffixes, const QString &prefix) = 0;
    virtual QString generateFileName(const QString &suffixes) = 0;
    virtual QString generateFileName(const QString &suffixes, const QString &prefix) = 0;

protected:
    //属性包装
    QString fileP;
    QString fileSuffixesP;
    QString userP;
    int versionP;
    QString filepathP;
    QString infoP;
    QString fileStateP;

protected:
    QString formatPath(const QString &value) const;
};

CORE_DNAMESPACE_E
