//
// Created by TD on 2022/9/29.
//

#pragma once
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/app_facet.h>
#include <doodle_core/doodle_core.h>

#include <doodle_app/app/authorization.h>
#include <doodle_app/app/facet/gui_facet.h>
#include <doodle_app/app/program_options.h>
#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/gui/main_proc_handle.h>

namespace doodle {

/**
 * @brief 基本的命令行类
 *  ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
 */
template <typename Facet_Defaute, typename... Facet_>
class app_command : public app_base {
 protected:
  bool chick_authorization() override {
    DOODLE_LOG_INFO("开始检查授权");
    return authorization{}.is_expire();
  };

 public:
  app_command() : app_base() {
    doodle_lib::Get().ctx().emplace<program_options>();

    add_facet<Facet_Defaute>();
    (add_facet<Facet_>(), ...);
  };
  virtual ~app_command() override = default;

  static app_command& Get() { return *(dynamic_cast<app_command*>(self)); }
  template <typename T>
  void add_facet() {
    static_cast<T*>(facet_list.emplace_back(std::in_place_type<T>).data());
  };

 protected:
  virtual void post_constructor() override {
    auto& l_opt = doodle_lib::Get().ctx().get<program_options>();
    for (auto&& val : facet_list) {
      val->add_program_options();
    }
    l_opt.command_line_parser();
    facet_list |= ranges::actions::remove_if([](app_facet_interface& in) { return !in->post(); });
  };

  virtual void deconstruction() override { facet_list.clear(); }
};

/**
 * @brief 基本的命令行类
 *  ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
 */
template <typename Facet_Defaute>
using app_plug = app_command<Facet_Defaute>;

}  // namespace doodle
