#pragma once


#include "core_global.h"
#include "coresql.h"
#include <QObject>
#include <QString>
#include <QFileInfo>
#include <QFile>
#include <QDir>

CORE_NAMESPACE_S

enum class dep
{
    None_,
    Executive,
    Light,
    VFX,
    modle,
    rig,
    Anm,
    direct,
    paint
};

struct synPath_struct{
    QString local;
    QString server;
};

typedef QVector<synPath_struct> synPathListPtr;

/*
*全局静态设置类
*/

class CORE_EXPORT coreSet :public QObject
{
    Q_OBJECT
public:
    static coreSet& getCoreSet();
    coreSet & operator =(const coreSet& s) =delete ;
    coreSet(const coreSet& s) = delete ;

    //初始化函数
    void init();
    void initdb();
    //获得同步路径
    synPathListPtr getSynDir();

    //ip设置
    QString getIpMysql() const;
    void setIpMysql(const QString &value);
    //user设置
    QString getUser() const;
    QString getUser_en() const;
    void setUser(const QString &value);
    //部门设置
    QString getDepartment() const;
    void setDepartment(const QString &value);
    //同步集数设置
    int getSyneps() const;
    void setSyneps(int value);
    //获得freesyn同步软件设置
    QFileInfo getFreeFileSyn() const;
    void setFreeFileSyn(const QFileInfo &value);
    //项目名称设置
    QString getProjectname() const;
    void setProjectname(const QString &value);
    //shot根路径
    QDir getShotRoot() const;
    void setShotRoot(const QDir &value);
    //ass根路径
    QDir getAssRoot() const;
    void setAssRoot(const QDir &value);
    //project根路径
    QDir getPrjectRoot() const;
    void setPrjectRoot(const QDir &value);
    //缓存路径
    QDir getCacheRoot() const;
    //doc路径
    QDir getDoc() const;


public slots:
    void writeDoodleLocalSet();



private:
    //私有化构造函数
    coreSet();
    //获得缓存磁盘路径
    void getCacheDiskPath();
    //获得服务器上的统一设置
    void getServerSetting();
    //获得本地的有限设置
    void getSetting();
    //转换为ip路径
    static QString toIpPath(const QString &path);
private:
    const static QString settingFileName;
    QString ipMysql;
    QString user;
    QString department;
    int syneps;
    QFileInfo freeFileSyn;
    QString projectname;

private:
    //内部属性
    QFileInfo synPath;
    QDir synServer;

    //只读属性
    QStringList ProgramFolder;
    QStringList assTypeFolder;
    QStringList Amnnnll;

    QDir shotRoot;
    QDir assRoot;
    QDir prjectRoot;

    QDir cacheRoot;
    QDir doc;
};

CORE_DNAMESPACE_E

