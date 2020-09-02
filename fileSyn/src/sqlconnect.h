#ifndef SQLCONNECT_H
#define SQLCONNECT_H
#include "fileSyn_global.h"
#include <QtSql/QSqlDatabase>
#include <QtCore/QSharedPointer>
#include <QtCore/QDir>
DNAMESPACE_S

typedef QSharedPointer<QSqlDatabase> sqldatabasePtr;

class FILESYN_EXPORT sqlConnect
{
public:
    ~sqlConnect();
    sqlConnect(const sqlConnect&) = delete;
    sqlConnect & operator = (const sqlConnect& s) = delete ;

    static sqlConnect& GetSqlConnect();

    bool openSqlDB(const QDir & dir);
    bool setRoot(const QDir & dir);
    bool createTable(const QString & sqlcom);
    bool subObj(QSqlQuery &query);
    QSqlDatabase &GetdataBase();
    void closedb();
private:
    sqlConnect();
    QSqlDatabase dataBase;
    QDir Root;
};

DNAMESPACE_E
#endif // SQLCONNECT_H
