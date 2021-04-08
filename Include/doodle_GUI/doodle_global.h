/*
 * @Author: your name
 * @Date: 2020-09-28 14:13:33
 * @LastEditTime: 2020-12-01 14:36:15
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\doodle_global.h
 */
#pragma once

#include <corelib/core_global.h>
#include <doodle_GUI/core_cpp_qt.h>

#include <QtCore/qglobal.h>
#include <QWidget>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSpinBox>

// class QHBoxLayout;
// class QComboBox;

DOODLE_NAMESPACE_S

enum class filterState {
  useFilter,
  notFilter,
  showAll,
};

class shotEpsListModel;
class episodesintDelegate;
class shotEpsListWidget;

class shotListModel;
class shotIntEnumDelegate;
class shotListWidget;

class shotClassModel;
class shotClassWidget;

class shotTypeModel;
class shotTypeWidget;

class shotTableModel;
class shotTableWidget;

class assDepModel;
class assDepWidget;

class assClassModel;
class assClassWidget;
class assSortfilterModel;

class assTypeModel;
class assTypeWidget;

class assTableModel;
class assTableWidght;

class SettingWidget;

class ScreenshotWidght;
class ScreenshotAction;

class shotWidget;
class assWidght;
class mainWindows;

DOODLE_NAMESPACE_E