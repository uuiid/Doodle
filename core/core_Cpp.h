/*
 * @Author: your name
 * @Date: 2020-11-13 13:45:54
 * @LastEditTime: 2020-11-26 16:06:43
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\core\ core_Cpp.h
 */
//
// Created by teXiao on 2020/11/13.
//
#pragma once

#include <core_global.h>

#include <src/shots/episodes.h>
#include <src/shots/shot.h>
#include <src/shots/shotClass.h>
#include <src/shots/shottype.h>
#include <src/shots/shotfilesqlinfo.h>

#include <src/assets/assdepartment.h>
#include <src/assets/assClass.h>
#include <src/assets/assType.h>
#include <src/assets/assfilesqlinfo.h>

#include <src/fileArchive/mayaArchive.h>
#include <src/fileArchive/mayaArchiveShotFbx.h>
#include <src/fileArchive/moveShotA.h>
#include <src/fileArchive/ueSynArchive.h>
#include <src/fileArchive/ueArchive.h>
#include <src/fileArchive/ueSynArchive.h>
#include <src/fileArchive/movieEpsArchive.h>
#include <src/fileArchive/imageArchive.h>

#include <src/core/coreDataManager.h>
#include <src/core/coreset.h>

#include <src/sysData/synData.h>

#include <src/queueData/queueManager.h>
#include <src/queueData/queueData.h>

#ifdef DOODLE_QT
#include <QtCore/QVariant>
Q_DECLARE_METATYPE(doodle::episodesPtr)
Q_DECLARE_METATYPE(doodle::shotPtr)
Q_DECLARE_METATYPE(doodle::shotClassPtr)
Q_DECLARE_METATYPE(doodle::shotTypePtr)
Q_DECLARE_METATYPE(doodle::shotInfoPtr)

Q_DECLARE_METATYPE(doodle::assDepPtr)
Q_DECLARE_METATYPE(doodle::assClassPtr)
Q_DECLARE_METATYPE(doodle::assTypePtr)
Q_DECLARE_METATYPE(doodle::assInfoPtr)

Q_DECLARE_METATYPE(doodle::queueDataPtr)
// Q_DECLARE_METATYPE(doodle::dataInfoPtr)

#define DOTOS(str) QString::fromStdString(str)
#endif  //DOODLE_QT