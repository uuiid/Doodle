//
// Created by TD on 2022/9/30.
//

#pragma once

#include <doodle_app/doodle_app_fwd.h>

#include <doodle_core/doodle_core.h>
#include <doodle_core/core/app_facet.h>

namespace doodle::facet {

class DOODLE_APP_API json_rpc_facet : public ::doodle::detail::app_facet_interface {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
  virtual void load_rpc_server() = 0;

 public:
  json_rpc_facet();
  virtual ~json_rpc_facet();
  const std::string& name() const noexcept override;
  void operator()() override;
  void deconstruction() override;
};

}  // namespace doodle
