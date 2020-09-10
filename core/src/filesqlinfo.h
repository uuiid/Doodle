#ifndef FILESQLINFO_H
#define FILESQLINFO_H

#include "core_global.h"
#include <QVector>
#include <QFileInfo>
#include <QSharedPointer>


CORE_NAMESPACE_S

class shot;
class episodes;
class fileClass;
class fileType;
class assType;
class fileSqlInfo;

typedef QSharedPointer<shot> shotPtr;
typedef QSharedPointer<episodes> episodesPtr;
typedef QSharedPointer<fileClass> fileClassPtr;
typedef QSharedPointer<fileType> fileTypePtr;
typedef QSharedPointer<assType> assTypePtr;
typedef QSharedDataPointer<fileSqlInfo> fileSqlInfoPtr;

typedef QSharedDataPointer<QVector<QFileInfo>> QfileListPtr;

class CORE_EXPORT fileSqlInfo
{
public:
    //属性设置和查询
    fileSqlInfo();
    QfileListPtr fileListGet() const;
    void setFileList(const QfileListPtr filelist);

protected:
    //属性包装
    qint64 idP;
    QString fileP;
    QString fileSuffixesP;
    QString userP;
    int versionP;
    QString filepathP;
    QString fileStateP;



public:
    //外键查询
    episodesPtr episdes() const;
    void setEpisdes( const episodesPtr& eps_);

    shotPtr shot() const;
    void setshot(const shotPtr& shot_);

    fileClassPtr fileclass() const;
    void setFileClass(const fileClassPtr& fileclass_);

    fileTypePtr fileType() const;
    void setFileType(const fileTypePtr& fileType_);

    assTypePtr assType() const;
    void setAssType(const assTypePtr& assType_);

protected:
    episodesPtr epsP;
    shotPtr shotP;
    fileClassPtr fileClassP;
    fileTypePtr fileTypeP;
    assTypePtr assTypeP;

};

class CORE_EXPORT shot {

};

CORE_DNAMESPACE_E
#endif // FILESQLINFO_H
