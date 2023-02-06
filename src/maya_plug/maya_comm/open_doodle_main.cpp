//
// Created by TD on 2021/12/13.
//
#include "open_doodle_main.h"

#ifdef fsin
#undef fsin
#endif

#include <doodle_core/core/app_facet.h>

#include <maya_plug/gui/maya_plug_app.h>

// #include <maya/MQtUtil.h>

namespace doodle::maya_plug {
open_doodle_main::open_doodle_main()  = default;

open_doodle_main::~open_doodle_main() = default;

MStatus open_doodle_main::doIt(const MArgList& list) {
  // auto l_o = MQtUtil::nativeWindow(MQtUtil::mainWindow());

  if (auto l_f = app_base::Get().find_facet<doodle::facet::gui_facet>(); l_f) {
    l_f->show_windows();
  }
  return MStatus::kSuccess;
}
}  // namespace doodle::maya_plug
