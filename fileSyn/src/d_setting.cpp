#include "d_setting.h"
#include <QUuid>
SYN_NAMESPACE_S
doFileSyn::d_setting::~d_setting()
{

}

d_setting &d_setting::GetSetting()
{
    static d_setting install;
    return install;
}

void d_setting::fastSetSynDef_syn()
{
    setSynDef_local_new(d_setting::synSet::updata);
    setSynDef_local_only(d_setting::synSet::updata);
    setSynDef_server_new(d_setting::synSet::down);
    setSynDef_server_only(d_setting::synSet::down);
    setSynDef_conflict(d_setting::synSet::ignore);
}

void d_setting::fastSetSynDef_down()
{
    setSynDef_local_new(d_setting::synSet::down);
    setSynDef_local_only(d_setting::synSet::down);
    setSynDef_server_new(d_setting::synSet::down);
    setSynDef_server_only(d_setting::synSet::down);
    setSynDef_conflict(d_setting::synSet::down);
}

void d_setting::fastSetSynDef_updata()
{
    setSynDef_local_new(d_setting::synSet::updata);
    setSynDef_local_only(d_setting::synSet::updata);
    setSynDef_server_new(d_setting::synSet::updata);
    setSynDef_server_only(d_setting::synSet::updata);
    setSynDef_conflict(d_setting::synSet::updata);
}

void d_setting::setSynDef_local_only(const d_setting::synSet &d)
{
    loacl_only_def = d;
}

void d_setting::setSynDef_local_new(const d_setting::synSet &d)
{
    local_new_def =d;
}

void d_setting::setSynDef_conflict(const d_setting::synSet &d)
{
    conflict_def = d;
}

void d_setting::setSynDef_server_only(const d_setting::synSet &d)
{
    server_only_def =d;
}

void d_setting::setSynDef_server_new(const d_setting::synSet &d)
{
    server_new_def = d;
}

d_setting::synSet d_setting::getSynDef_local_only()
{
    return loacl_only_def;
}

d_setting::synSet d_setting::getSynDef_local_new()
{
    return local_new_def;
}

d_setting::synSet d_setting::getSynDef_conflict()
{
    return conflict_def;
}

d_setting::synSet d_setting::getSynDef_server_only()
{
    return server_only_def;
}

d_setting::synSet d_setting::getSynDef_server_new()
{
    return server_new_def;
}


void d_setting::setBackup(const QUrl &backUp)
{
    backUpPath = backUp;
}

QUrl d_setting::getBackup(const QDir & path)
{
    QString folder("%1/%2");
    QDir p(path);
    p.cdUp();
    if(!synTime.isNull()){
        folder = folder.arg(synTime.toString(),p.path());
    }else {
        folder = folder.arg(QUuid::createUuid().toString(QUuid::Id128),p.path());
    }
    if(backUpPath.isLocalFile()){
        QDir(backUpPath.toLocalFile()).mkpath(folder);
    }else {

    }
    folder = QString("%1/%2/%3").arg(backUpPath.toString(),folder,path.path());
    return QUrl(folder);
}





d_setting::d_setting()
{

}
DNAMESPACE_E
