//
// Created by TD on 2022/10/8.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <doodle_app/app/facet/json_rpc_facet.h>

namespace doodle::facet {

class DOODLELIB_API rpc_server_facet : public ::doodle::detail::app_facet_interface{
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  rpc_server_facet();
  virtual ~rpc_server_facet() override;

  std::shared_ptr<::doodle::json_rpc::server> server_attr() const;

  const std::string& name() const noexcept override;
  void operator()() override;
  void deconstruction() override;
};

}  // namespace doodle::facet
