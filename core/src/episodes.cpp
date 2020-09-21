#include "episodes.h"

#include "sql_builder/sql.h"
#include "coreset.h"

#include <QVariant>
#include <QSqlError>

CORE_NAMESPACE_S

episodes::episodes()
{
    episP = -1;
}

episodes::episodes(const qint64 &ID_)
{
    sql::SelectModel sel_;

    sel_.select("id","episodes");

    sel_.from(QString("%1.episodes").arg(coreSet::getCoreSet().getProjectname()).toStdString());
    sel_.where(sql::column("id") == ID_);

    sqlQuertPtr query = coreSql::getCoreSql().getquery();
    if (!query->exec(QString::fromStdString(sel_.str()))) return;
    if(query->next()){
        idP           = query->value(0).toInt();
        episP         = query->value(1).toInt();
        return;
    }
    idP = -1;
}

void episodes::insert()
{
    sql::InsertModel ins_;
    if (idP < 0){
        sqlQuertPtr query = coreSql::getCoreSql().getquery();

        ins_.insert("episodes", episP);

        ins_.into(QString("%1.episodes").arg(coreSet::getCoreSet().getProjectname()).toStdString());

        if(!query->exec(QString::fromStdString(ins_.str())))
            throw std::runtime_error(query->lastError().text().toStdString());

        query->finish();
    }else{
        updateSQL();
    }
}
void episodes::updateSQL()
{
    return;
}

void episodes::deleteSQL()
{

}

void episodes::setEpisdes(const qint64 & value)
{
    episP = value;
}

qint64 episodes::getEpisdes() const
{
    return episP;
}
CORE_DNAMESPACE_E
