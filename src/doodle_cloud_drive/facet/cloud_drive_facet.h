//
// Created by td_main on 2023/7/5.
//

#pragma once

#include <doodle_core/core/file_sys.h>
#include <doodle_core/doodle_core_fwd.h>

#include "boost/asio/signal_set.hpp"

#include <doodle_cloud_drive/cloud_drive.h>
#include <memory>
#include <nlohmann/json.hpp>
namespace doodle {

class cloud_drive_arg {
 public:
  FSys::path root_path{};
  FSys::path server_path{};

  friend void to_json(nlohmann::json& j, const cloud_drive_arg& p) {
    j["root_path"]   = p.root_path;
    j["server_path"] = p.server_path;
  };

  friend void from_json(const nlohmann::json& j, cloud_drive_arg& p) {
    j.at("root_path").get_to(p.root_path);
    j.at("server_path").get_to(p.server_path);
  };

  constexpr static const char* name = "cloud_drive_config";
};

class cloud_drive_facet {
  std::shared_ptr<cloud_provider_registrar> registrar_{};
  decltype(boost::asio::make_work_guard(g_io_context())) work_guard_{boost::asio::make_work_guard(g_io_context())};
  boost::asio::signal_set signals_{g_io_context(), SIGINT, SIGTERM};

 public:
  bool post();
};

}  // namespace doodle
