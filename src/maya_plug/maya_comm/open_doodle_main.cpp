//
// Created by TD on 2021/12/13.
//

// clang-format off
//#include <QtWidgets/QWidget>
//#include <QtWidgets/QPushButton>
//#include <maya/MQtUtil.h>
// clang-format on

#include "open_doodle_main.h"

#ifdef fsin
#undef fsin
#endif

#include <doodle_core/core/app_facet.h>

#include <doodle_app/gui/base/base_window.h>

#include <maya_plug/gui/maya_plug_app.h>

namespace doodle::maya_plug {
open_doodle_main::open_doodle_main()  = default;

open_doodle_main::~open_doodle_main() = default;

MStatus open_doodle_main::doIt(const MArgList& list) {
  // auto l_o = MQtUtil::nativeWindow(MQtUtil::mainWindow());
  //  MStatus l_status{};
  //  MString l_self = MGlobal::executeCommandStringResult(R"(global string $gShelfTopLevel; $tmp = $gShelfTopLevel;)");
  //
  //  auto l_layout  = MQtUtil::findLayout(l_self);
  //
  //  auto l_sub     = new QWidget{};
  //  l_sub->setObjectName("doodle_test_1");
  //
  //  auto l_b = new QPushButton{l_sub};
  //  l_b->setText("tset");
  //
  //  auto l_str = MQtUtil::addWidgetToMayaLayout(l_sub, l_layout);
  //  displayWarning(l_str);
  //  l_sub->show();
  //  auto l_list = MQtUtil::getLayoutChildren(l_layout);
  //
  //  for (auto&& i : l_list) {
  //    displayWarning(MQtUtil::fullName(i));
  //    auto name = i->metaObject()->className();
  //    displayWarning(name);
  //  }

  gui::g_windows_manage().show_windows();
  return MStatus::kSuccess;
}
}  // namespace doodle::maya_plug
