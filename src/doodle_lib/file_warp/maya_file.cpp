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
void maya_file_async::export_fbx_file(const std::vector<FSys::path>& in_vector,
                                      const details::export_fbx_arg& in_arg,
                                      const std::int32_t in_max_th) {
  std::vector<l_data2> l_data_vector;
  for (auto& k_path : in_vector) {
    l_data2 l_data1{};
    l_data1.p_handle = make_handle();

    l_data1.p_handle.emplace<process_message>().set_name(k_path.filename().generic_string());
    l_data1.arg           = in_arg;
    l_data1.arg.file_path = k_path;
    l_data_vector.push_back(std::move(l_data1));
  }
  g_main_loop().attach(
      [in_data = l_data_vector, in_max_th](auto delta, void*, auto succeed, auto fail) mutable {
        auto k_run_size = std::count_if(in_data.begin(), in_data.end(), [&](auto&& in_e) {
          return in_e.p_handle.get<process_message>().is_run();
        });

        if (k_run_size >= in_max_th)
          return;

        for (int l_i = 0; l_i < (in_max_th - k_run_size); ++l_i) {
          auto& l_item = in_data.front();
          if (l_item.p_handle.template get<process_message>().is_run())
            break;
          g_main_loop().attach<details::maya_exe>(l_item.p_handle, l_item.arg);
          std::rotate(in_data.begin(), in_data.begin() + 1, in_data.end());
        }
        if (std::any_of(in_data.begin(), in_data.end(), [&](auto&& in_item) {
              return !in_item.p_handle.get<process_message>().is_wait();
            })) {
          succeed();
        }
      });
}
void maya_file_async::qcloth_sim_file(const std::vector<FSys::path>& in_vector,
                                      const details::qcloth_arg& in_arg,
                                      const std::int32_t in_max_th) {
  std::vector<l_data> l_data_vector;
  for (auto& k_path : in_vector) {
    l_data l_data1{};
    l_data1.p_handle = make_handle();

    l_data1.p_handle.emplace<process_message>().set_name(k_path.filename().generic_string());
    l_data1.arg          = in_arg;
    l_data1.arg.sim_path = k_path;
    l_data_vector.push_back(std::move(l_data1));
  }
  g_main_loop().attach(
      [in_data = l_data_vector, in_max_th](auto delta, void*, auto succeed, auto fail) mutable {
        auto k_run_size = std::count_if(in_data.begin(), in_data.end(), [&](auto&& in_e) {
          return in_e.p_handle.get<process_message>().is_run();
        });

        if (k_run_size > in_max_th)
          return;

        for (int l_i = 0; l_i < (in_max_th - k_run_size + 1); ++l_i) {
          auto& l_item = in_data.front();
          if (l_item.p_handle.template get<process_message>().is_run())
            break;
          g_main_loop().attach<details::maya_exe>(l_item.p_handle, l_item.arg);

          std::rotate(in_data.begin(), in_data.begin() + 1, in_data.end());
        }
        if (std::all_of(in_data.begin(), in_data.end(), [&](auto&& in_item) {
              return !in_item.p_handle.get<process_message>().is_wait();
            })) {
          succeed();
        }
      });
}

}  // namespace doodle
