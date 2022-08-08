//
// Created by TD on 2022/8/8.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/user.h>

namespace doodle {
namespace detail {

class DOODLE_CORE_EXPORT user_set_data {
 public:
  user_set_data()          = default;
  virtual ~user_set_data() = default;

  database data_ref{};
  ::doodle::business::rules rules_attr{};
  user user_data{};
};

}  // namespace detail
}  // namespace doodle
