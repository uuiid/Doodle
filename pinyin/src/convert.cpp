#include "convert.h"
#include <QSqlError>

#include <QDebug>
#include <QTemporaryFile>

PINYIN_NAMESPACE_S

QTemporaryFile convert::tmpDBFile;

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
    if (!isinitDB)
        initDB();
    QRegularExpressionMatchIterator iter = re->globalMatch(conStr);
    QString enStr = conStr;
    while (iter.hasNext())
    {
        QRegularExpressionMatch matchStr = iter.next();
        enStr = enStr.replace(matchStr.captured(), toEnOne(matchStr.captured()));
    }
    return enStr;
}

void convert::initDB()
{
    if (!createDB())
        throw std::runtime_error("not search DB");
    if (QSqlDatabase::contains("sqlite_pinyin_db"))
    {
        dataBase = QSqlDatabase::database("sqlite_pinyin_db");
    }
    else
    {
        dataBase = QSqlDatabase::addDatabase("QSQLITE", "sqlite_pinyin_db");
        dataBase.setDatabaseName(tmpDBFile.fileName());
    }
    if (!dataBase.open())
        throw std::runtime_error(dataBase.lastError().text().toStdString());
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

bool convert::createDB()
{
    if (tmpDBFile.exists())
        return true;
    if (tmpDBFile.open())
    {
        QFile tmp(":/pinyin/pinyin.db");
        if (tmp.open(QIODevice::ReadOnly))
        {
            tmpDBFile.write(tmp.readAll());
        }
        tmp.close();
    }
    else
    {
        return false;
    }
    tmpDBFile.close();
    return true;
}

QString convert::toEnOne(const QString &conStr)
{
    QString sql = "SELECT en FROM pinyin WHERE znch='%1';";
    query->prepare(sql.arg(conStr));
    if (!query->exec())
        throw std::runtime_error(QString("not quert %1").arg(conStr).toStdString());
    QString enstr = "";
    if (query->next())
    {
        enstr = query->value(0).toString();
    }
    return enstr;
}

DNAMESPACE_E
