#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/core/filesystem_extend.h>
#include <doodle_lib/file_warp/maya_file.h>
#include <doodle_lib/lib_warp/boost_locale_warp.h>
#include <doodle_lib/lib_warp/std_warp.h>
#include <doodle_lib/thread_pool/long_term.h>
#include <doodle_lib/thread_pool/thread_pool.h>
#include <doodle_lib/metadata/metadata.h>
#include <doodle_lib/thread_pool/thread_pool.h>
#include <boost/locale.hpp>
#include <boost/process.hpp>
#ifdef _WIN32
#include <boost/process/windows.hpp>

#elif defined __linux__
#include <boost/process/posix.hpp>

#endif
#include <doodle_lib/exe_warp/maya_exe.h>
namespace doodle {

maya_file_async::maya_file_async() {}
namespace {
struct l_data {
  entt::handle p_handle;
  details::qcloth_arg arg;
};
struct l_data2 {
  entt::handle p_handle;
  details::export_fbx_arg arg;
};

}  // namespace
void maya_file_async::export_fbx_file(const entt::handle& in_handle, const details::export_fbx_arg& in_arg) {
  if (!in_handle.any_of<process_message>())
    in_handle.emplace<process_message>();

  in_handle.patch<process_message>([&](process_message& in) {
    in.set_name(in_arg.file_path.filename().generic_string());
  });

  chick_true<doodle_error>(!in_arg.file_path.empty(), DOODLE_LOC, "没有文件");
  g_bounded_pool().attach<details::maya_exe>(in_handle, in_arg);
}
void maya_file_async::qcloth_sim_file(const entt::handle& in_handle, const details::qcloth_arg& in_arg) {
  if (!in_handle.any_of<process_message>())
    in_handle.emplace<process_message>();

  in_handle.patch<process_message>([&](process_message& in) {
    in.set_name(in_arg.sim_path.filename().generic_string());
  });

  chick_true<doodle_error>(!in_arg.sim_path.empty(), DOODLE_LOC, "没有文件");
  g_bounded_pool().attach<details::maya_exe>(in_handle, in_arg);
}

}  // namespace doodle
