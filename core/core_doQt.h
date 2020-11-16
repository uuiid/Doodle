//
// Created by teXiao on 2020/11/13.
//
#pragma once

#include <core_global.h>

#include <src/episodes.h>
#include <src/shot.h>
#include <src/shotClass.h>
#include <src/shottype.h>
#include <src/shotfilesqlinfo.h>

#include <src/assdepartment.h>
#include <src/assClass.h>
#include <src/assType.h>
#include <src/assfilesqlinfo.h>

#include <src/mayaArchive.h>
#include <src/mayaArchiveShotFbx.h>
#include <src/moveShotA.h>
#include <src/ueSynArchive.h>
#include <src/ueArchive.h>

#include <src/coreDataManager.h>
#include <src/coreset.h>

#ifdef DOODLE_QT
#include <QtCore/QVariant>
Q_DECLARE_METATYPE(doCore::episodesPtr)
Q_DECLARE_METATYPE(doCore::shotPtr)
Q_DECLARE_METATYPE(doCore::shotClassPtr)
Q_DECLARE_METATYPE(doCore::shotTypePtr)
Q_DECLARE_METATYPE(doCore::shotInfoPtr)

Q_DECLARE_METATYPE(doCore::assDepPtr)
Q_DECLARE_METATYPE(doCore::assClassPtr)
Q_DECLARE_METATYPE(doCore::assTypePtr)
Q_DECLARE_METATYPE(doCore::assInfoPtr)
#define DOTOS(str) QString::fromStdString(str)
#endif //DOODLE_QT