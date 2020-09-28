#pragma once

#include "doodle_global.h"
#include "core_global.h"

#include <QAbstractListModel>
DOODLE_NAMESPACE_S

class episodesListWidget : public QAbstractListModel
{
    Q_OBJECT
private:
    doCore::episodesPtrList eplist;
public:
    episodesListWidget(/* args */);
    ~episodesListWidget();
};


DOODLE_NAMESPACE_E