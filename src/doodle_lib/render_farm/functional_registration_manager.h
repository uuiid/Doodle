//
// Created by td_main on 2023/9/19.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle::render_farm {

class functional_registration_manager {
 private:
  std::map<std::string, std::function<nlohmann::json(const nlohmann::json&)>> map_;

 public:
  functional_registration_manager()                                                      = default;
  ~functional_registration_manager()                                                     = default;

  // copy
  functional_registration_manager(const functional_registration_manager&)                = delete;
  functional_registration_manager& operator=(const functional_registration_manager&)     = delete;
  // move
  functional_registration_manager(functional_registration_manager&&) noexcept            = default;
  functional_registration_manager& operator=(functional_registration_manager&&) noexcept = default;

  void register_function(std::string in_name, std::function<nlohmann::json(const nlohmann::json&)> in_function);

  std::function<nlohmann::json(const nlohmann::json&)> get_function(std::string in_name);
};

}  // namespace doodle::render_farm
