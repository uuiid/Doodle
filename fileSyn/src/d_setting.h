#ifndef D_SETTING_H
#define D_SETTING_H
#include "fileSyn_global.h"
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QUrl>

DNAMESPACE_S

class FILESYN_EXPORT d_setting
{
public:
    ~d_setting();
    d_setting(const d_setting&) =delete ;
    d_setting & operator =(const d_setting& s) =delete ;
    enum class synSet{
        down,
        updata,
        ignore,
        del,
    };
    enum class fileState{
        ch_1,
        ch_2,
        conflict,
        agree
    };

    static d_setting& GetSetting();

    void fastSetSynDef_syn();
    void fastSetSynDef_down();
    void fastSetSynDef_updata();

    void setSynDef_local_only(const synSet & d);
    void setSynDef_local_new(const synSet & d);
    void setSynDef_conflict(const synSet & d);
    void setSynDef_server_only(const synSet & d);
    void setSynDef_server_new(const synSet & d);

    synSet getSynDef_local_only();
    synSet getSynDef_local_new();
    synSet getSynDef_conflict();
    synSet getSynDef_server_only();
    synSet getSynDef_server_new();


    void setBackup(const QUrl &backUp);
    QUrl getBackup(const QDir &path);
protected:
    synSet loacl_only_def;
    synSet local_new_def;
    synSet conflict_def;
    synSet server_only_def;
    synSet server_new_def;

    QDateTime synTime;
    QUrl backUpPath;

private:
    d_setting();
};

DNAMESPACE_E
#endif // D_SETTING_H
