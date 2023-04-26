//
// Created by td_main on 2023/4/26.
//

#include "maya_lib_guard.h"

#include <maya/MLibrary.h>
namespace doodle {
namespace maya_plug {
maya_lib_guard::maya_lib_guard() { MLibrary::initialize(true, "maya_doodle"); }
maya_lib_guard::~maya_lib_guard() { MLibrary::cleanup(0, false); }
}  // namespace maya_plug
}  // namespace doodle