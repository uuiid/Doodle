/*
 * @Author: your name
 * @Date: 2020-09-28 14:13:33
 * @LastEditTime: 2020-10-09 15:06:20
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\doodle_GUI\doodle_global.h
 */
#pragma once



#include <QtCore/qglobal.h>
#include <QWidget>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSpinBox>


#define DOODLE_NAMESPACE_S namespace doodle {
#define DOODLE_NAMESPACE_E };

// class QHBoxLayout;
// class QComboBox;

DOODLE_NAMESPACE_S

class episodesListModel;
class episodesintDelegate;
class episodesListWidget;

class shotListModel;
class shotIntEnumDelegate;
class shotListWidget;

class fileClassShotModel;
class fileClassShotWidget;

class fileTypeShotModel;
class fileTypeShotWidget;

class shotTableModel;
class shotTableWidget;

class fileClassAssModel;
class fileClassAssWidget;

class assClassModel;
class assClassWidget;

class fileTypeAssModel;
class fileTypeAssWidget;

class assTableModel;
class assTableWidght;

class settingWidget;

class ProjectWidget;
class mainWindows;

DOODLE_NAMESPACE_E