//
// Created by TD on 2022/10/8.
//
#pragma once

#include <doodle_core/core/app_facet.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::facet {

class DOODLELIB_API rpc_server_facet : public ::doodle::detail::app_facet_interface {
  doodle::distributed_computing::server_ptr server_attr;

  std::string name_{"json_rpc"};

  std::shared_ptr<decltype(boost::asio::make_work_guard(g_io_context()))> work_{};

 public:
  rpc_server_facet();
  virtual ~rpc_server_facet() override;

  const std::string& name() const noexcept override;
  void operator()() override;
  void deconstruction() override;
};

}  // namespace doodle::facet
