//
// Created by TD on 2022/10/8.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <doodle_app/app/facet/json_rpc_facet.h>


namespace doodle::facet {

class DOODLELIB_API rpc_server_facet : public json_rpc_facet {

  class impl;
  std::unique_ptr<impl> p_i;
 public:
  rpc_server_facet();
  void load_rpc_server() override;
};

}  // namespace doodle::facet
