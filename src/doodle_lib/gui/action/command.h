//
// Created by TD on 2021/9/18.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/hana.hpp>
namespace doodle {

namespace details {
enum class command_type {
  simple,
  async,
  composite
};
}

class DOODLELIB_API command_base /* : public details::no_copy  */ {
 protected:
  std::string p_name;
  std::map<string, string> p_show_str;
  using metadata_variant = std::variant<episodes_ptr, shot_ptr, season_ptr, assets_ptr, assets_file_ptr>;
  metadata_variant p_meta_var;

 public:
  virtual const std::string& class_name() { return p_name; };
  virtual bool is_async() { return false; };
  virtual bool render() = 0;

  template <class... Args>
  bool add_data(const Args&... in_args) {
    auto k_arg            = boost::hana::make_tuple(in_args...);
    auto constexpr k_size = decltype(boost::hana::size(k_arg))::value;

    if constexpr (k_size == 0) {
      return false;
    } else if (k_size == 2) {
      return true;
    }

    return false;
  };
};

// using command_tool = command_base<>;
// using command_meta = command_base<const metadata_ptr&, const metadata_ptr&>;

}  // namespace doodle
