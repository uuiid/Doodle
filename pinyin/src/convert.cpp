#include "convert.h"
#include <QSqlError>

#include <QDebug>

PINYIN_NAMESPACE_S
convert::convert()
{
    isinitDB = false;
    re = QSharedPointer<QRegularExpression>(new QRegularExpression());
}

convert::~convert()
{
    dataBase.close();
}

QString convert::toEn(const QString &conStr)
{
    if(!isinitDB) initDB();
    QRegularExpressionMatchIterator iter= re->globalMatch(conStr);
    QString enStr = conStr;
    while (iter.hasNext()) {
        QRegularExpressionMatch matchStr = iter.next();
        enStr = enStr.replace(matchStr.captured(),toEnOne(matchStr.captured()));
    }
    return enStr;
}

void convert::initDB()
{
    if(!search()) throw std::runtime_error("not search DB");
    if(QSqlDatabase::contains("sqlite_pinyin_db")){
        dataBase = QSqlDatabase::database("sqlite_pinyin_db");
    }else {
        dataBase = QSqlDatabase::addDatabase("QSQLITE","sqlite_pinyin_db");
//        dataBase.setDatabaseName("pinyin/pinyin.db");
        dataBase.setDatabaseName(sqlDBPath->absolutePath() + "/pinyin.db");
    }
    if(!dataBase.open()) throw std::runtime_error(dataBase.lastError().text().toStdString());
    dataBase.transaction();
    initQuery();
    initExp();
    isinitDB = true;
}

void convert::initQuery()
{
    query = QSharedPointer<QSqlQuery>(new QSqlQuery(dataBase));
}

void convert::initExp()
{
    re->setPattern("[\u4e00-\u9fa5]");
}

bool convert::search()
{
    QDir dir(QDir::currentPath());
    qDebug() << dir.absolutePath();
    dir.cdUp();
    if(!dir.cd("resource")) return false;
    sqlDBPath = QSharedPointer<QDir>(new QDir(dir));
    return true;
}

QString convert::toEnOne(const QString &conStr)
{
    QString sql = "SELECT en FROM pinyin WHERE znch='%1';";
    query->prepare(sql.arg(conStr));
    if(!query->exec()) throw std::runtime_error(QString("not quert %1").arg(conStr).toStdString());
    QString enstr="";
    if(query->next()){
        enstr = query->value(0).toString();
    }
    return enstr;
}

DNAMESPACE_E
