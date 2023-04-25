//
// Created by TD on 2022/10/8.
//
#pragma once

#include <doodle_core/core/app_facet.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::facet {

class DOODLELIB_API rpc_server_facet final {
  doodle::distributed_computing::server_ptr server_attr;

  std::string name_{"json_rpc"};

  std::shared_ptr<decltype(boost::asio::make_work_guard(g_io_context()))> work_{};

 public:
  rpc_server_facet() = default;
  virtual ~rpc_server_facet();

  const std::string& name() const noexcept;
  bool post();
  void add_program_options(){};
};

}  // namespace doodle::facet
