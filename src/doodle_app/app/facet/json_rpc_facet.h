//
// Created by TD on 2022/9/30.
//

#pragma once

#include <doodle_app/doodle_app_fwd.h>

#include <doodle_core/doodle_core.h>
#include <doodle_core/core/app_facet.h>

namespace doodle {
namespace facet {

class DOODLE_APP_API json_rpc_facet : public ::doodle::detail::app_facet_interface {
  std::string name_attr;
 public:
  json_rpc_facet();
  const std::string& name() const noexcept override;
  void operator()() override;
  void deconstruction() override;
};

}  // namespace facet
}  // namespace doodle
