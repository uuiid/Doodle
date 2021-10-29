//
// Created by TD on 2021/9/18.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <any>
#include <boost/hana.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/free.hpp>
#include <boost/type_erasure/member.hpp>
#include <boost/type_erasure/operators.hpp>

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
  entt::handle p_meta_var;

  registry_ptr reg;

 public:
  command_base();
  virtual const std::string& class_name() { return p_name; };
  virtual bool is_async() { return false; };
  virtual bool render()                             = 0;
  virtual bool set_data(const entt::handle& in_any) = 0;
  virtual bool set_parent(const entt::handle& in_ptr);
};

class DOODLELIB_API command_base_tool : public command_base {
  bool set_data(const entt::handle& in_any) override { return false; };
  bool set_parent(const entt::handle& in_ptr) override { return false; };

 public:
  command_base_tool() = default;
};

class DOODLELIB_API command_base_list : public command_base {
 public:
  std::vector<std::shared_ptr<command_base>> p_list;

  std::vector<std::shared_ptr<command_base>>& get() {
    return p_list;
  }

  command_base_list() = default;
  bool render() override;
  bool set_data(const entt::handle& in_any) override;
  bool set_parent(const entt::handle& in_ptr) override;
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
};

class command_interface
    : public entt::type_list<
          bool()> {
 public:
  command_interface() = default;
  template <typename Base>
  class type : public Base {
   public:
    bool render() {
      return entt::poly_call<0>(*this);
    }
  };
  template <typename Type>
  using impl = entt::value_list<&Type::render>;
};

BOOST_TYPE_ERASURE_MEMBER(render);

using command_ = boost::type_erasure::any<
    boost::mpl::vector<
        has_render<bool()>,
        boost::type_erasure::copy_constructible<>,
        boost::type_erasure::typeid_<>,
        boost::type_erasure::relaxed>>;

// using command_ = entt::poly<command_interface>;

}  // namespace doodle
