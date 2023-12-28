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

#include <array>
#include <tuple>

namespace doodle {

/**
 * @brief 基本的命令行类
 */
template <typename... Facet_>
class app_command : public app_base {
 public:
  app_command() : app_base() { run_facet(); };

  app_command(int argc, const char* const argv[]) : app_command(argc, argv) { run_facet(); }
  virtual ~app_command() override = default;

  void run_facet() {
    std::array<bool, sizeof...(Facet_)> l_r{
        Facet_{}(arg_, facets_)...,
    };
    stop_ = std::any_of(l_r.begin(), l_r.end(), [](bool i) { return i; });
  }

 protected:
};

template <typename... Facet_>
using app_plug = app_command<Facet_...>;

}  // namespace doodle
