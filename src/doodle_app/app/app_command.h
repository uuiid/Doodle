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

#include <tuple>

namespace doodle {

/**
 * @brief 基本的命令行类
 *  ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
 */
template <typename Facet_Defaute, typename... Facet_>
class app_command : public app_base {
 public:
  app_command() : app_base() {
    g_ctx().emplace<program_options>();

    add_facet<Facet_Defaute>();
    (add_facet<Facet_>(), ...);
  };

  app_command(int argc, const char* const argv[]) : app_command() {
    for (auto&& val : facet_list) {
      val->add_program_options();
    }
    g_ctx().get<program_options>().arg.parse(argc, argv);
  }
  virtual ~app_command() override = default;

  static app_command& Get() { return *(dynamic_cast<app_command*>(self)); }
  template <typename T>
  auto add_facet() {
    return static_cast<T*>(facet_list.emplace_back(std::in_place_type<T>).data());
  };

 protected:
  virtual void post_constructor() override {
    facet_list |= ranges::actions::remove_if([](app_facet_interface& in) { return !in->post(); });
  };

  virtual void deconstruction() override { facet_list.clear(); }
};

template <typename Facet_Defaute>
using app_plug = app_command<Facet_Defaute>;

}  // namespace doodle
