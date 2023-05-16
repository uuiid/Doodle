//
// Created by TD on 2021/10/18.
//

#include "program_options.h"

#include "doodle_core/core/file_sys.h"
#include "doodle_core/database_task/sqlite_client.h"
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/logger/logger.h>

#include <doodle_app/lib_warp/boost_fmt_progrm_opt.h>

#include "boost/asio/detail/handler_type_requirements.hpp"
#include <boost/locale.hpp>

namespace doodle::details {

void program_options::init_project() const {
  if (auto l_path = FSys::from_quotation_marks(arg(1).str());
      !l_path.empty() && FSys::exists(l_path) && FSys::is_regular_file(l_path) &&
      l_path.extension() == doodle_config::doodle_db_name.data()) {
    doodle_lib::Get().ctx().get<database_n::file_translator_ptr>()->async_open(l_path, [](bsys::error_code) -> void {
      DOODLE_LOG_INFO("完成打开项目");
    });
  } else if (l_path = core_set::get_set().project_root[0]; !l_path.empty() && FSys::exists(l_path) &&
                                                           FSys::is_regular_file(l_path) &&
                                                           l_path.extension() == doodle_config::doodle_db_name.data()) {
    doodle_lib::Get().ctx().get<database_n::file_translator_ptr>()->async_open(l_path, [](bsys::error_code) -> void {
      DOODLE_LOG_INFO("完成打开项目");
    });
  }
}

}  // namespace doodle::details
