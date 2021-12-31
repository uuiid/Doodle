//
// Created by TD on 2021/9/18.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/entt_warp.h>
#include <any>
#include <boost/hana.hpp>

namespace doodle {

namespace details {
enum class command_type {
  simple,
  async,
  composite
};

}  // namespace details

class DOODLELIB_API command_base /* : public details::no_copy  */ {
 protected:
  std::string p_name;
  std::map<string, string> p_show_str;

 public:
  command_base();
  virtual const std::string& class_name() { return p_name; };
  ;
  virtual bool render() = 0;
  virtual bool set_data(const entt::handle& in_any) { return false; };
};

template <class... arg>
class DOODLELIB_API command_list {
 public:
  boost::hana::tuple<arg...> list;

  command_list() : list(){};

  bool render() {
    bool k_r{true};
    boost::hana::for_each(
        list,
        [&](auto& in) {
          dear::TreeNode{in.class_name().c_str()} && [&]() {
            k_r &= in.render();
          };
        });
    return k_r;
  };

  bool set_data(const entt::handle& in_any) {
    bool k_r{true};
    boost::hana::for_each(
        list,
        [&](auto& in) {
          k_r &= in.set_data(in_any);
        });
    return k_r;
  }
};

// using command_ = entt::poly<command_interface>;

}  // namespace doodle
