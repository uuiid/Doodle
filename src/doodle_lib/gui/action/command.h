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
  using metadata_variant = std::variant<
      episodes_ptr,
      shot_ptr,
      season_ptr,
      assets_ptr,
      assets_file_ptr,
      project_ptr,
      std::nullptr_t>;
  metadata_ptr p_meta_var;

  virtual bool set_child(const episodes_ptr& in_ptr) { return false; };
  virtual bool set_child(const shot_ptr& in_ptr) { return false; };
  virtual bool set_child(const season_ptr& in_ptr) { return false; };
  virtual bool set_child(const assets_ptr& in_ptr) { return false; };
  virtual bool set_child(const assets_file_ptr& in_ptr) { return false; };
  virtual bool set_child(const project_ptr& in_ptr) { return false; };
  virtual bool set_child(const std::nullptr_t& in_ptr) { return false; };

 public:
  virtual const std::string& class_name() { return p_name; };
  virtual bool is_async() { return false; };
  virtual bool render() = 0;

  template <class... Args>
  bool add_data(const Args&... in_args) {
    using namespace boost::hana::literals;
    auto k_arg            = boost::hana::make_tuple(in_args...);
    auto constexpr k_size = decltype(boost::hana::size(k_arg))::value;

    if constexpr (k_size == 0) {
      return false;
    } else if (k_size == 2) {
      p_meta_var = k_arg[0_c];
      return this->set_child(k_arg[1_c]);
    }
    return false;
  };
};

// using command_tool = command_base<>;
// using command_meta = command_base<const metadata_ptr&, const metadata_ptr&>;

}  // namespace doodle
