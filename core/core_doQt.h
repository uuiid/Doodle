//
// Created by teXiao on 2020/11/13.
//
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
Q_DECLARE_METATYPE(doCore::episodes)
Q_DECLARE_METATYPE(doCore::shot)
Q_DECLARE_METATYPE(doCore::shotClass)
Q_DECLARE_METATYPE(doCore::shotType)
Q_DECLARE_METATYPE(doCore::shotFileSqlInfo)

Q_DECLARE_METATYPE(doCore::assdepartment)
Q_DECLARE_METATYPE(doCore::assClass)
Q_DECLARE_METATYPE(doCore::assType)
Q_DECLARE_METATYPE(doCore::assFileSqlInfo)
#define DOTOS(str) QString::fromStdString(str)
#endif //DOODLE_QT