#include "crashpad.h"

#include "doodle_core/core/core_set.h"
#include <doodle_core/doodle_core.h>

#include <boost/dll/runtime_symbol_info.hpp>

#include <base/files/file_path.h>
#include <crashpad/client/crashpad_client.h>
#include <crashpad/client/settings.h>
#include <crashpad/client/simple_string_dictionary.h>
#include <crashpad/util/misc/uuid.h>
#include <spdlog/spdlog.h>

namespace doodle {
crashpad_init::crashpad_init() {
  static bool g_initialized = false;
  if (g_initialized) return;
  g_initialized = true;

  crashpad::CrashpadClient l_client{};
  FSys::path l_db_path = core_set::get_set().get_cache_root("crashpad_db");
  FSys::create_directories(l_db_path);
  FSys::path l_handler_path = boost::dll::program_location().parent_path() / "crashpad_handler.exe";

  crashpad::UUID l_uuid{};
  std::map<std::string, std::string> l_annotations{};
  l_annotations["version"] = version::build_info::get().version_str;
  std::vector<std::string> l_arguments{};
  l_arguments.push_back("--no-rate-limit");

  bool l_success = l_client.StartHandler(
      base::FilePath{l_handler_path.wstring()}, base::FilePath{l_db_path.wstring()},
      base::FilePath{l_db_path.wstring()}, std::string{}, l_annotations, l_arguments, true, false
  );
  if (!l_success) {
    SPDLOG_ERROR(fmt::format("crashpad 初始化失败, handler: {}, db: {}", l_handler_path.string(), l_db_path.string()));
  } else {
    SPDLOG_INFO(fmt::format("crashpad 初始化成功, handler: {}, db: {}", l_handler_path.string(), l_db_path.string()));
  }
}
crashpad_init::~crashpad_init() {}
crashpad_init& crashpad_init::get() {
  static crashpad_init g_instance{};
  return g_instance;
}
}  // namespace doodle