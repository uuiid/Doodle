//
// Created by TD on 2021/9/18.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {

namespace details {
enum class command_type {
  simple,
  async,
  composite
};
}

template <class... Args>
class DOODLELIB_API command_base /* : public details::no_copy  */{
 protected:
  std::string p_name;
  std::map<string, string> p_show_str;

 public:
  virtual const std::string& class_name() { return p_name; };
  virtual bool is_async() { return false; };
  virtual bool render() {
    ImGui::BulletText(p_name.c_str());
    return false;
  };
  virtual bool add_data(const Args&... in_args) { return false; };
};

using command_tool = command_base<>;
using command_meta = command_base<const metadata_ptr&, const metadata_ptr&>;

}  // namespace doodle
