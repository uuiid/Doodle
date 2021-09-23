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
class DOODLELIB_API command_base : public details::no_copy {
 protected:
  std::string p_name;

 public:
  virtual const std::string& class_name() { return p_name; };
  virtual bool is_async() { return false; };
  virtual bool run(const Args&... in_args) { return false; };
};

using command_tool = command_base<>;
using command_meta = command_base<MetadataPtr,MetadataPtr>;

}  // namespace doodle
