//
// Created by TD on 2022/8/8.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/user.h>

#include <rttr/rttr_enable.h>

namespace doodle {
namespace detail {

class user_set_data;
void DOODLE_CORE_API to_json(nlohmann::json& j, const user_set_data& p);
void DOODLE_CORE_API from_json(const nlohmann::json& j, user_set_data& p);
class DOODLE_CORE_API user_set_data {
  RTTR_ENABLE();

 public:
  user_set_data()          = default;
  virtual ~user_set_data() = default;

  database data_ref{};
  ::doodle::business::rules rules_attr{};
  user user_data{};

 private:
  friend void to_json(nlohmann::json& j, const user_set_data& p);
  friend void from_json(const nlohmann::json& j, user_set_data& p);
};

}  // namespace detail
}  // namespace doodle
