//
// Created by TD on 2022/5/11.
//
#include "file_sys.h"

#include <nlohmann/json.hpp>
namespace doodle::FSys {
void to_json(nlohmann::json& j, const path_u8& p) {
  j = p.generic_string();
};
void from_json(const nlohmann::json& j, path_u8& p) {
  p = j.get<std::string>();
};

}  // namespace doodle::FSys
