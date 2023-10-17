//
// Created by TD on 2022/10/13.
//

#include "null_facet.h"

#include <doodle_core/database_task/sqlite_client.h>

#include <doodle_app/app/program_options.h>

#include <maya_plug/data/maya_create_movie.h>
namespace doodle {
namespace maya_plug {
const std::string& null_facet::name() const noexcept {
  static std::string l_i{"null_facet"};
  return l_i;
}
bool null_facet::post() {
  g_ctx().get<doodle::database_n::file_translator_ptr>()->set_only_ctx(true);
  g_ctx().get<program_options>().init_project();
  return true;
}
void null_facet::deconstruction() { work_lock.reset(); }
null_facet::null_facet() { g_reg()->ctx().emplace<image_to_move>() = std::make_shared<detail::maya_create_movie>(); }
}  // namespace maya_plug
}  // namespace doodle