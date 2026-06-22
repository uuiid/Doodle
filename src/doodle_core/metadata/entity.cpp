//
// Created by TD on 25-7-17.
//
#include "entity.h"

namespace doodle {
std::int32_t entity_asset_extend_value::extract_number_from_name(const std::string& in_name) {
  // 使用正则表达式提取字符串中的数字
  const static std::regex l_regex("\\d+");
  std::smatch l_match;
  if (std::regex_search(in_name, l_match, l_regex)) {
    return std::stoi(l_match.str());
  }
  return 0;  // 如果没有找到数字，返回0或其他默认值
}
}  // namespace doodle