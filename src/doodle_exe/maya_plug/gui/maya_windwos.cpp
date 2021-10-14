//
// Created by TD on 2021/10/14.
//

#include "maya_windwos.h"

#include <maya/MFileIO.h>
#include <maya_plug/gui/maya_plug_app.h>
namespace doodle::maya_plug {

bool maya_windwos::maya_tool() {


  return false;
}

maya_windwos::maya_windwos()
    : main_windows() {
}

void maya_windwos::frame_render() {
  main_windows::frame_render();
}
}  // namespace doodle::maya_plug