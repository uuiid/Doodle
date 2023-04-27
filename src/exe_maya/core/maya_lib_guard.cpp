//
// Created by td_main on 2023/4/26.
//

#include "maya_lib_guard.h"

#include <doodle_core/doodle_core.h>
#include <doodle_core/logger/logger.h>

#include <maya_plug/logger/maya_logger_info.h>

#include <maya/MLibrary.h>
#include <memory>

// #include <boost/align.hpp>
namespace doodle {
namespace maya_plug {
maya_lib_guard::maya_lib_guard() {
  MLibrary::initialize(true, "maya_doodle");
  doodle::g_logger_ctrl().add_log_sink(std::make_shared<::doodle::maya_plug::maya_msg_mt>(), "maya_plug");
}
maya_lib_guard::~maya_lib_guard() { MLibrary::cleanup(0, false); }
}  // namespace maya_plug
}  // namespace doodle