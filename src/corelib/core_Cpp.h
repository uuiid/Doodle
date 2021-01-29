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

#include <corelib/core_global.h>

#include <corelib/shots/episodes.h>
#include <corelib/shots/shot.h>
#include <corelib/shots/shotClass.h>
#include <corelib/shots/shottype.h>
#include <corelib/shots/shotfilesqlinfo.h>

#include <corelib/assets/assdepartment.h>
#include <corelib/assets/assClass.h>
#include <corelib/assets/assType.h>
#include <corelib/assets/assfilesqlinfo.h>

#include <corelib/fileArchive/mayaArchive.h>
#include <corelib/fileArchive/mayaArchiveShotFbx.h>
#include <corelib/fileArchive/moveShotA.h>
#include <corelib/fileArchive/ueSynArchive.h>
#include <corelib/fileArchive/ueArchive.h>
#include <corelib/fileArchive/ueSynArchive.h>
#include <corelib/fileArchive/movieEpsArchive.h>
#include <corelib/fileArchive/imageArchive.h>

#include <corelib/core/coreDataManager.h>
#include <corelib/core/coreset.h>

#include <corelib/sysData/synData.h>

#include <corelib/fileArchive/ScreenshotArchive.h>

#include <corelib/queueData/queueManager.h>
#include <corelib/queueData/queueData.h>

#ifdef DOODLE_QT
#include <QtCore/QVariant>
Q_DECLARE_METATYPE(doodle::episodes *)
Q_DECLARE_METATYPE(doodle::shot *)
Q_DECLARE_METATYPE(doodle::shotClass *)
Q_DECLARE_METATYPE(doodle::shotType *)
Q_DECLARE_METATYPE(doodle::shotFileSqlInfo *)

Q_DECLARE_METATYPE(doodle::assdepartment *)
Q_DECLARE_METATYPE(doodle::assClass *)
Q_DECLARE_METATYPE(doodle::assType *)
Q_DECLARE_METATYPE(doodle::assFileSqlInfo *)

Q_DECLARE_METATYPE(doodle::queueData *)
// Q_DECLARE_METATYPE(doodle::dataInfoPtr)

#define DOTOS(str) QString::fromStdString(str)
#endif  //DOODLE_QT