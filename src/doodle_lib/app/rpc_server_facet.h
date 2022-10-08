//
// Created by TD on 2022/10/8.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <doodle_app/app/facet/gui_facet.h>

namespace doodle {
namespace facet {

class DOODLELIB_API json_rpc_facet : public rpc_server_facet {
 public:
  json_rpc_facet();
  void load_rpc_server() override;
};

}  // namespace facet
}  // namespace doodle
