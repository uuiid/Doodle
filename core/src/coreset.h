#ifndef CORESET_H
#define CORESET_H

#include "core_global.h"
#include "coresql.h"
#include <QObject>
#include <QString>
#include <QFileInfo>
#include <QFile>
#include <QDir>

CORE_NAMESPACE_S


class CORE_EXPORT coreSet :public QObject
{
    Q_OBJECT
public:
    static coreSet& getCoreSet();
    coreSet & operator =(const coreSet& s) =delete ;
    coreSet(const coreSet& s) = delete ;



private:
    coreSet();
    void getSetting();
private:
    const static QString settingFileName;
    QString user;
    QString department;
    QFileInfo synPath;
    int syneps;
    QFileInfo freeFileSyn;
    QString prjectname;
    QStringList ProgramFolder;
    QStringList assTypeFolder;
    QStringList Amnnnll;
    QString project;
    QDir doc;
};

CORE_DNAMESPACE_E
#endif // CORESET_H
