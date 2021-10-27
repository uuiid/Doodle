//
// Created by TD on 2021/9/18.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

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
  entt::entity p_meta_var;

 public:
  virtual const std::string& class_name() { return p_name; };
  virtual bool is_async() { return false; };
  virtual bool render()                         = 0;
  virtual bool set_data(const std::any& in_any) = 0;
  virtual bool set_parent(const entt::entity& in_ptr);
};

class DOODLELIB_API command_base_tool : public command_base {
  bool set_data(const std::any& in_any) override { return false; };
  bool set_parent(const entt::entity& in_ptr) override { return false; };

 public:
  command_base_tool() = default;
};

class DOODLELIB_API command_base_list : public command_base {
 public:
  std::vector<std::shared_ptr<command_base>> p_list;

  command_base_list() = default;
  bool render() override;
  bool set_data(const std::any& in_any) override;
  bool set_parent(const entt::entity& in_ptr) override;
};
}  // namespace doodle
