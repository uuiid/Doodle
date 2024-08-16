//
// Created by TD on 2023/12/23.
//

#include "scan_category_service.h"
namespace doodle::details {

void scan_category_service_t::init_logger_data() {
  logger_      = std::make_shared<spdlog::logger>("scan_category");
  logger_->sinks().emplace_back(g_logger_ctrl().rotating_file_sink_);
}

}  // namespace doodle::details