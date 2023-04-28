//
// Created by td_main on 2023/4/28.
//

#pragma once
#include <maya/MString.h>
#include <string>

namespace doodle::maya_plug {
namespace conv {
inline std::string to_s(const MString& in_string) { return in_string.asUTF8(); }
inline MString to_ms(const std::string& in_string) {
  MString k_r{};
  k_r.setUTF8(in_string.c_str());
  return k_r;
}

}  // namespace conv

/**
 * @brief  字符串转换类
 * 作为maya字符串和标准库字符串之间的转换
 */
class d_str {
 public:
  std::string p_u8_str{};
  template <class MStr, std::enable_if_t<std::is_same_v<MString, MStr>, bool> = true>
  explicit d_str(const MStr& in) : p_u8_str(in.asUTF8()){};

  //  explicit d_str(const MString& in)
  //      : p_u8_str(in.asUTF8()){};

  explicit d_str(std::string in_u8_str) : p_u8_str(std::move(in_u8_str)) {}

  inline operator MString() const {
    MString k_r{};
    k_r.setUTF8(p_u8_str.c_str());
    return k_r;
  }
  inline operator std::string() const { return p_u8_str; }
  [[nodiscard]] inline std::string str() const { return p_u8_str; }
};
}  // namespace doodle::maya_plug