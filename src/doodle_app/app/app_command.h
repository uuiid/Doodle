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
    program_options::emplace();
    auto run_facet         = std::make_shared<Facet_Defaute>();
    default_run_facet_name = run_facet->name();
    add_facet(run_facet);
    (add_facet(std::make_shared<Facet_>()), ...);
  };
  virtual ~app_command() override { program_options::reset(); }

  static app_command& Get() { return *(dynamic_cast<app_command*>(self)); }
  void add_facet(const app_facet_ptr& in_facet) {
    program_options::value().build_opt(in_facet->name());
    facet_list.emplace(in_facet->name(), in_facet);
  };

 protected:
  virtual void post_constructor() override {
    auto& l_opt = doodle::program_options::value();

    for (auto&& [key, val] : facet_list) {
      val->add_program_options();
    }
    l_opt.command_line_parser();

    bool run_facet{};
    for (auto&& [key, val] : l_opt.facet_model) {
      if (val) {
        DOODLE_LOG_INFO("开始运行 {} facet", key);
        boost::asio::post(g_io_context(), [l_f = facet_list.at(key)]() { (*l_f)(); });
        run_facet = true;
      }
    }

    if (!run_facet) {
      DOODLE_LOG_INFO("运行默认构面 {}", default_run_facet_name);
      l_opt.facet_model[default_run_facet_name] = true;
      boost::asio::post(g_io_context(), [l_f = facet_list.at(default_run_facet_name)]() { (*l_f)(); });
    }
  };

  virtual void deconstruction() override {
    auto& l_opt = doodle::program_options::value();
    for (auto&& [key, val] : l_opt.facet_model) {
      if (val) {
        DOODLE_LOG_INFO("结束 {} facet", key);
        facet_list.at(key)->deconstruction();
      }
    }
  }
};

/**
 * @brief 基本的命令行类
 *  ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
 */
template <typename Facet_Defaute>
class app_plug : public app_base {
 protected:
  bool chick_authorization() override {
    DOODLE_LOG_INFO("开始检查授权");
    return authorization{}.is_expire();
  };

 public:
  app_plug() : app_base() {
    auto run_facet         = std::make_shared<Facet_Defaute>();
    default_run_facet_name = run_facet->name();
    add_facet(run_facet);
  };
  virtual ~app_plug() override = default;

  void add_facet(const app_facet_ptr& in_facet) { facet_list.emplace(in_facet->name(), in_facet); };

 protected:
  virtual void post_constructor() override {
    DOODLE_LOG_INFO("运行默认构面 {}", default_run_facet_name);

    boost::asio::post(g_io_context(), [l_f = facet_list.at(default_run_facet_name)]() { (*l_f)(); });
  };

  virtual void deconstruction() override { facet_list.at(default_run_facet_name)->deconstruction(); }
};
}  // namespace doodle
