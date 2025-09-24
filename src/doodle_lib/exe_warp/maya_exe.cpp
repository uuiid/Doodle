//
// Created by TD on 2021/12/25.
//

#include "maya_exe.h"

#include "doodle_core/core/file_sys.h"
#include "doodle_core/core/global_function.h"
#include <doodle_core/configure/config.h>
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/core/filesystem_extend.h>
#include <doodle_lib/exe_warp/async_read_pipe.h>
#include <doodle_lib/exe_warp/windows_hide.h>

#include <boost/asio.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/process/process.hpp>

#include <filesystem>
#include <fmt/core.h>
#include <string>
#include <winnt.h>
#include <winreg/WinReg.hpp>

namespace doodle {
namespace {
FSys::path find_maya_path_impl() {
  try {
    auto l_key_str = fmt::format(LR"(SOFTWARE\Autodesk\Maya\{}\Setup\InstallPath)", core_set::get_set().maya_version);
    winreg::RegKey l_key{};
    l_key.Open(HKEY_LOCAL_MACHINE, l_key_str, KEY_QUERY_VALUE | KEY_WOW64_64KEY);
    auto l_maya_path = l_key.GetStringValue(LR"(MAYA_INSTALL_LOCATION)");
    return {l_maya_path};
  } catch (const winreg::RegException& in_err) {
    throw_exception(doodle_error{"寻找maya路径失败: {}", in_err.what()});
  }
  return {};
}

FSys::path install_maya_exe(FSys::path in_maya_path) {
  static bool is_run{false};
  FSys::path l_out{};
  auto l_target_path = FSys::get_cache_path() / "maya" / "exe" / fmt::to_string(core_set::get_set().maya_version) /
                       version::build_info::get().version_str;
  const auto l_run_name = fmt::format("doodle_maya_exe_{}.exe", core_set::get_set().maya_version);
  l_out                 = l_target_path / l_run_name;
  if (is_run) return l_out;

  if (!FSys::exists(l_target_path)) FSys::create_directories(l_target_path);

  FSys::copy(
      in_maya_path / "bin" / "ShadeFragment", l_target_path / "ShadeFragment",
      FSys::copy_options::recursive | FSys::copy_options::update_existing
  );

  FSys::copy(
      in_maya_path / "bin" / "ScriptFragment", l_target_path / "ScriptFragment",
      FSys::copy_options::recursive | FSys::copy_options::update_existing
  );

  FSys::copy(
      register_file_type::program_location() / l_run_name, l_target_path / l_run_name,
      FSys::copy_options::update_existing
  );

  auto l_program_path = register_file_type::program_location();
  for (auto&& l_it : FSys::directory_iterator(l_program_path)) {
    if (l_it.is_regular_file() && l_it.path().extension() == ".dll") {
      auto l_path_dll = l_target_path / l_it.path().filename();
      FSys::copy(l_it, l_path_dll, FSys::copy_options::update_existing);
    }
  }
  is_run = true;
  return l_out;
}

void add_maya_module() {
  static std::string const k_mod{R"(+ doodle 1.1 .
MYMODULE_LOCATION:= .
PATH+:= plug-ins
PYTHONPATH+:= scripts
)"};
  auto l_maya_plug = register_file_type::program_location().parent_path() / "maya";

  if (!FSys::exists(l_maya_plug / "doodle.mod")) {
    FSys::ofstream k_file{l_maya_plug / "doodle.mod"};
    k_file << k_mod;
  }
}

maya_exe_ns::maya_out_arg get_out_arg_impl(const FSys::path& in_path) {
  if (!FSys::exists(in_path)) return {};

  std::ifstream l_file{in_path};
  auto l_str  = std::string{std::istreambuf_iterator<char>(l_file), std::istreambuf_iterator<char>()};
  auto l_json = nlohmann::json::parse(l_str);
  return l_json.get<maya_exe_ns::maya_out_arg>();
}
}  // namespace

namespace maya_exe_ns {

void from_json(const nlohmann::json& nlohmann_json_j, maya_out_arg::out_file_t& nlohmann_json_t) {
  nlohmann_json_j["out_file"].get_to(nlohmann_json_t.out_file);
  nlohmann_json_j["ref_file"].get_to(nlohmann_json_t.ref_file);
  if (nlohmann_json_j.contains("hide_material_list"))
    nlohmann_json_j.at("hide_material_list").get_to(nlohmann_json_t.hide_material_list);
};

void to_json(nlohmann::json& nlohmann_json_j, const maya_out_arg::out_file_t& nlohmann_json_t) {
  nlohmann_json_j["out_file"]           = nlohmann_json_t.out_file.generic_string();
  nlohmann_json_j["ref_file"]           = nlohmann_json_t.ref_file.generic_string();
  nlohmann_json_j["hide_material_list"] = nlohmann_json_t.hide_material_list;
};
void from_json(const nlohmann::json& nlohmann_json_j, maya_out_arg& nlohmann_json_t) {
  nlohmann_json_j["begin_time"].get_to(nlohmann_json_t.begin_time);
  nlohmann_json_j["end_time"].get_to(nlohmann_json_t.end_time);
  nlohmann_json_j["out_file_list"].get_to(nlohmann_json_t.out_file_list);
};

void to_json(nlohmann::json& nlohmann_json_j, const maya_out_arg& nlohmann_json_t) {
  nlohmann_json_j["begin_time"]    = nlohmann_json_t.begin_time;
  nlohmann_json_j["end_time"]      = nlohmann_json_t.end_time;
  nlohmann_json_j["out_file_list"] = nlohmann_json_t.out_file_list;
};

// form json
void from_json(const nlohmann::json& in_json, arg& out_obj) {
  if (in_json.contains("path")) in_json.at("path").get_to(out_obj.file_path);
  if (in_json.contains("out_path_file")) in_json.at("out_path_file").get_to(out_obj.out_path_file_);
}
// to json
void to_json(nlohmann::json& in_json, const arg& out_obj) {
  in_json["path"]          = out_obj.file_path;
  in_json["out_path_file"] = out_obj.out_path_file_;
}

void from_json(const nlohmann::json& in_json, qcloth_arg& out_obj) {
  from_json(in_json, static_cast<maya_exe_ns::arg&>(out_obj));
  if (in_json.contains("sim_path")) in_json.at("sim_path").get_to(out_obj.sim_path);
  if (in_json.contains("replace_ref_file")) in_json.at("replace_ref_file").get_to(out_obj.replace_ref_file);
  if (in_json.contains("sim_file")) in_json.at("sim_file").get_to(out_obj.sim_file);
  if (in_json.contains("export_file")) in_json.at("export_file").get_to(out_obj.export_file);
  if (in_json.contains("touch_sim")) in_json.at("touch_sim").get_to(out_obj.touch_sim);
  if (in_json.contains("export_anim_file")) in_json.at("export_anim_file").get_to(out_obj.export_anim_file);
  if (in_json.contains("create_play_blast")) in_json.at("create_play_blast").get_to(out_obj.create_play_blast_);
  if (in_json.contains("camera_film_aperture"))
    in_json.at("camera_film_aperture").get_to(out_obj.film_aperture_);
  else
    out_obj.film_aperture_ = 1.78;
  if (in_json.contains("image_size")) in_json.at("image_size").get_to(out_obj.size_);
}
// to json
void to_json(nlohmann::json& in_json, const qcloth_arg& out_obj) {
  to_json(in_json, static_cast<const maya_exe_ns::arg&>(out_obj));
  in_json["sim_path"]             = out_obj.sim_path.generic_string();
  in_json["replace_ref_file"]     = out_obj.replace_ref_file;
  in_json["sim_file"]             = out_obj.sim_file;
  in_json["export_file"]          = out_obj.export_file;
  in_json["touch_sim"]            = out_obj.touch_sim;
  in_json["export_anim_file"]     = out_obj.export_anim_file;
  in_json["create_play_blast"]    = out_obj.create_play_blast_;
  in_json["camera_film_aperture"] = out_obj.film_aperture_;
  in_json["image_size"]           = out_obj.size_;
}

void from_json(const nlohmann::json& in_json, export_fbx_arg& out_obj) {
  from_json(in_json, static_cast<maya_exe_ns::arg&>(out_obj));

  if (in_json.contains("create_play_blast")) in_json.at("create_play_blast").get_to(out_obj.create_play_blast_);
  if (in_json.contains("rig_file_export")) in_json.at("rig_file_export").get_to(out_obj.rig_file_export_);
  if (in_json.contains("camera_film_aperture"))
    in_json.at("camera_film_aperture").get_to(out_obj.film_aperture_);
  else
    out_obj.film_aperture_ = 1.78;
  if (in_json.contains("image_size")) in_json.at("image_size").get_to(out_obj.size_);
}
// to json
void to_json(nlohmann::json& in_json, const export_fbx_arg& out_obj) {
  to_json(in_json, static_cast<const maya_exe_ns::arg&>(out_obj));

  in_json["create_play_blast"]    = out_obj.create_play_blast_;
  in_json["rig_file_export"]      = out_obj.rig_file_export_;
  in_json["camera_film_aperture"] = out_obj.film_aperture_;
  in_json["image_size"]           = out_obj.size_;
}

void from_json(const nlohmann::json& in_json, replace_file_arg& out_obj) {
  from_json(in_json, static_cast<maya_exe_ns::arg&>(out_obj));

  if (in_json.contains("file_list")) in_json.at("file_list").get_to(out_obj.file_list);
}
// to json
void to_json(nlohmann::json& in_json, const replace_file_arg& out_obj) {
  to_json(in_json, static_cast<const maya_exe_ns::arg&>(out_obj));

  in_json["file_list"] = out_obj.file_list;
}
void to_json(nlohmann::json& in_json, const export_rig_arg& out_obj) {
  to_json(in_json, static_cast<const maya_exe_ns::arg&>(out_obj));
}

// form json
void from_json(const nlohmann::json& in_json, inspect_file_arg& out_obj) {
  from_json(in_json, static_cast<maya_exe_ns::arg&>(out_obj));

  if (in_json.contains("surface_5")) in_json.at("surface_5").get_to(out_obj.surface_5_);
  if (in_json.contains("rename_check")) in_json.at("rename_check").get_to(out_obj.rename_check_);
  if (in_json.contains("name_length_check")) in_json.at("name_length_check").get_to(out_obj.name_length_check_);
  if (in_json.contains("history_check")) in_json.at("history_check").get_to(out_obj.history_check_);
  if (in_json.contains("special_copy_check")) in_json.at("special_copy_check").get_to(out_obj.special_copy_check_);
  if (in_json.contains("uv_check")) in_json.at("uv_check").get_to(out_obj.uv_check_);
  if (in_json.contains("kframe_check")) in_json.at("kframe_check").get_to(out_obj.kframe_check_);
  if (in_json.contains("space_name_check")) in_json.at("space_name_check").get_to(out_obj.space_name_check_);
  if (in_json.contains("only_default_camera_check"))
    in_json.at("only_default_camera_check").get_to(out_obj.only_default_camera_check_);
  if (in_json.contains("too_many_point_check"))
    in_json.at("too_many_point_check").get_to(out_obj.too_many_point_check_);
  if (in_json.contains("multi_uv_inspection")) in_json.at("multi_uv_inspection").get_to(out_obj.multi_uv_inspection_);
}
// to json
void to_json(nlohmann::json& in_json, const inspect_file_arg& out_obj) {
  to_json(in_json, static_cast<const maya_exe_ns::arg&>(out_obj));

  in_json["surface_5"]                 = out_obj.surface_5_;
  in_json["rename_check"]              = out_obj.rename_check_;
  in_json["name_length_check"]         = out_obj.name_length_check_;
  in_json["history_check"]             = out_obj.history_check_;
  in_json["special_copy_check"]        = out_obj.special_copy_check_;
  in_json["uv_check"]                  = out_obj.uv_check_;
  in_json["kframe_check"]              = out_obj.kframe_check_;
  in_json["space_name_check"]          = out_obj.space_name_check_;
  in_json["only_default_camera_check"] = out_obj.only_default_camera_check_;
  in_json["too_many_point_check"]      = out_obj.too_many_point_check_;
}

FSys::path find_maya_path() { return find_maya_path_impl(); }

boost::asio::awaitable<void> arg::async_run_maya() {
  auto l_g = co_await g_ctx().get<maya_ctx>().queue_->queue(boost::asio::use_awaitable);
  logger_ptr_->warn("开始运行maya");
  if (time_info_) time_info_->start_time_ = std::chrono::system_clock::now();
  auto l_maya_path = find_maya_path_impl();

  auto l_this_exe  = co_await boost::asio::this_coro::executor;
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  auto l_run_path = install_maya_exe(l_maya_path);
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));

  out_path_file_ = FSys::get_cache_path() / "maya" / "out" / version::build_info::get().version_str /
                   fmt::format("{}.json", core_set::get_set().get_uuid());

  auto [l_key, l_args] = get_json_str();
  if (!FSys::exists(out_path_file_.parent_path())) FSys::create_directories(out_path_file_.parent_path());

  auto l_arg_path = FSys::write_tmp_file("maya/arg", l_args, ".json");

  logger_ptr_->warn("配置命令行 {}", l_arg_path);

  auto l_timer = std::make_shared<boost::asio::high_resolution_timer>(g_io_context());

  std::unordered_map<boost::process::v2::environment::key, boost::process::v2::environment::value> l_env{};
  for (auto&& l_it : boost::process::v2::environment::current()) {
    if (l_it.key() != L"PYTHONHOME" && l_it.key() != L"PYTHONPATH") l_env.emplace(l_it.key(), l_it.value());
  }

  l_env[L"MAYA_LOCATION"] = l_maya_path.generic_wstring();
  l_env[L"Path"].push_back((l_maya_path / "bin").generic_wstring());
  l_env[L"Path"].push_back(l_run_path.parent_path().generic_wstring());
  l_env[L"MAYA_MODULE_PATH"] = (register_file_type::program_location().parent_path() / "maya").generic_wstring();
  add_maya_module();
  auto l_out_pipe     = boost::asio::readable_pipe{co_await boost::asio::this_coro::executor};
  auto l_err_pipe     = boost::asio::readable_pipe{co_await boost::asio::this_coro::executor};

  auto l_process_maya = boost::process::v2::process{
      g_io_context(),
      l_run_path,
      {fmt::format("--{}", l_key), fmt::format("--config={}", l_arg_path)},
      boost::process::v2::process_stdio{nullptr, l_out_pipe, l_err_pipe},
      boost::process::v2::process_environment{l_env},
      boost::process::v2::process_start_dir{l_maya_path / "bin"},
      details::hide_and_not_create_windows
  };

  l_timer->expires_after(chrono::seconds{core_set::get_set().timeout});
  auto [l_array_completion_order, l_ec, l_exit_code, l_ec_t, l_ec1, l_ec2] =
      co_await boost::asio::experimental::make_parallel_group(
          boost::process::v2::async_execute(std::move(l_process_maya), boost::asio::deferred),
          l_timer->async_wait(boost::asio::deferred),
          async_read_pipe(l_out_pipe, logger_ptr_, boost::asio::deferred, level::info),
          async_read_pipe(l_err_pipe, logger_ptr_, boost::asio::deferred, level::info)
      )
          .async_wait(boost::asio::experimental::wait_for_one(), boost::asio::as_tuple(boost::asio::use_awaitable));
  if (time_info_) time_info_->end_time_ = std::chrono::system_clock::now();
  switch (l_array_completion_order[0]) {
    case 0:
      if (l_exit_code != 0 || l_ec) {
        switch (maya_enum::maya_error_t{l_exit_code}) {
          case maya_enum::maya_error_t::unknown_error:
            throw_exception(doodle_error{"maya 运行未知错误"});
            break;
          case maya_enum::maya_error_t::camera_name_error:
            throw_exception(doodle_error{"maya 中没有正确的 camera 名字"});
            break;
          case maya_enum::maya_error_t::bone_scale_error:
            throw_exception(doodle_error{"maya 中骨骼有缩放值为 0 的情况"});
            break;
          case maya_enum::maya_error_t::camera_aspect_error:
            throw_exception(doodle_error{"maya 中摄像机的宽高比不正确"});
            break;
          case maya_enum::maya_error_t::cache_path_error:
            throw_exception(doodle_error{"maya 中解算缓存路径不存在"});
            break;
          case maya_enum::maya_error_t::check_error:
            throw_exception(doodle_error{"maya 文件中有错误, 具体请点击查看日志"});
          default:
            throw_exception(doodle_error{"maya 运行未知错误 {}", l_exit_code});
        }
      }
      break;
    case 1:
      throw_exception(doodle_error{"maya 运行超时 {}", l_ec.message()});
    default:
      break;
  }
  out_arg_ = get_out_arg_impl(out_path_file_);
  co_return;
}

boost::asio::awaitable<void> qcloth_arg::run() { return arg::async_run_maya(); }
boost::asio::awaitable<void> export_fbx_arg::run() { return arg::async_run_maya(); }
boost::asio::awaitable<void> replace_file_arg::run() { return arg::async_run_maya(); }
boost::asio::awaitable<void> export_rig_arg::run() { return arg::async_run_maya(); }
boost::asio::awaitable<void> inspect_file_arg::run() { return arg::async_run_maya(); }

}  // namespace maya_exe_ns

}  // namespace doodle