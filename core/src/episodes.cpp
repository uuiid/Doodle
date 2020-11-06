#include "episodes.h"


#include "coreset.h"
#include "doException/doException.h"


CORE_NAMESPACE_S

episodes::episodes():p_int_episodes(-1)
{
}

void episodes::select(const qint64 &ID_)
{
//    sql::SelectModel sel_;
//
//    sel_.select("id", "episodes");
//
//    sel_.from(QString("%1.episodes").arg(coreSet::getCoreSet().getProjectname()).toStdString());
//    sel_.where(sql::column("id") == ID_);
//
//    sqlQuertPtr query = coreSql::getCoreSql().getquery();
//    if (!query->exec(QString::fromStdString(sel_.str())))
//        return;
//    if (query->next())
//    {
//        idP = query->value(0).toInt();
//        p_int_episodes = query->value(1).toInt();
//        return;
//    }
//    idP = -1;
}

void episodes::insert()
{
//    sql::InsertModel ins_;
//    if (idP < 0)
//    {
//        sqlQuertPtr query = coreSql::getCoreSql().getquery();
//        if (p_int_episodes < 0)
//            throw doodle_InsertErrorInfo(QString("%1").arg(p_int_episodes).toStdString());
//        ins_.insert("episodes", p_int_episodes);
//
//        ins_.into(QString("%1.episodes").arg(coreSet::getCoreSet().getProjectname()).toStdString());
//
//        if (!query->exec(QString::fromStdString(ins_.str())))
//            throw std::runtime_error(query->lastError().text().toStdString());
//        getInsertID(query);
//        query->finish();
//    }
}
void episodes::updateSQL()
{
    return;
}

void episodes::deleteSQL()
{
}

episodesPtrList episodes::getAll()
{
//    sql::SelectModel sel_;
//
//    sel_.select("id", "episodes");
//
//    sel_.from(QString("%1.episodes").arg(coreSet::getCoreSet().getProjectname()).toStdString());
//
//    sel_.order_by("episodes");
//
//    sqlQuertPtr query = coreSql::getCoreSql().getquery();
//    if (!query->exec(QString::fromStdString(sel_.str())))
//    {
//        throw std::runtime_error("not exe episode get all ");
//    }
//    episodesPtrList list_episode;
//    while (query->next())
//    {
//        episodesPtr tmp_eps(new episodes);
//        tmp_eps->idP = query->value(0).toInt();
//        tmp_eps->p_int_episodes = query->value(1).toInt();
//        list_episode.append(tmp_eps);
//    }
//    return list_episode;
}

void episodes::setEpisdes(const qint64 &value)
{
    p_int_episodes = value;
}

int64_t episodes::getEpisdes() const
{
    return p_int_episodes;
}

dstring episodes::getEpisdes_str() const
{
    QString str("ep%1");
    return str.arg(p_int_episodes, 3, 10, QLatin1Char('0'));
}

CORE_NAMESPACE_E
