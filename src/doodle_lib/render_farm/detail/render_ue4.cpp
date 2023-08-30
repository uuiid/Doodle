//
// Created by td_main on 2023/8/4.
//

#include "render_ue4.h"

#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/render_farm/work.h>

#include <boost/asio.hpp>
namespace doodle::render_farm {
namespace detail {

void render_ue4::run() {
  auto&& l_msg = self_handle_.get_or_emplace<process_message>();
  l_msg.message("开始下载ue4项目文件");
  auto& l_map = g_reg()->ctx().emplace<std::map<std::string, decltype(boost::asio::make_strand(g_thread()))>>();

  if (l_map.count(arg_.ProjectPath) == 0) {
    l_map.emplace(arg_.ProjectPath, boost::asio::make_strand(g_thread()));
  }

  strand_ = l_map.at(arg_.ProjectPath);
  server_file_path /= arg_.out_file_path;
  server_file_path = FSys::path{arg_.ProjectPath}.parent_path() / arg_.out_file_path;
  boost::asio::post(strand_, [this]() {
    bool l_r;
    try {
      l_r = download_file("D:/doodle/cache/ue");
    } catch (const doodle_error& e) {
      l_r = false;
      DOODLE_LOG_ERROR(e);
    } catch (...) {
      l_r = false;
      DOODLE_LOG_ERROR(boost::current_exception_diagnostic_information());
    }
    boost::asio::post(g_io_context(), [this, l_r]() { this->run_impl(l_r); });
  });
}
bool render_ue4::download_file(const FSys::path& in_file_path) {
  constexpr auto g_config     = "Config";
  constexpr auto g_content    = "Content";

  FSys::path const l_prj_path = arg_.ProjectPath;
  if (!FSys::exists(l_prj_path)) {
    return false;
  }

  const auto l_loc = in_file_path / l_prj_path.stem() / l_prj_path.filename();
  if (!FSys::exists(l_loc.parent_path())) FSys::create_directories(l_loc.parent_path());
  {  // 复制项目文件
    FSys::copy(l_prj_path, l_loc, FSys::copy_options::overwrite_existing);
  }
  {
    // 复制配置文件夹
    FSys::copy(
        l_prj_path.parent_path() / g_config, l_loc.parent_path() / g_config,
        FSys::copy_options::overwrite_existing | FSys::copy_options::recursive
    );
  }
  {
    // 复制内容文件夹
    auto l_loc_content = l_loc.parent_path() / g_content;
    auto l_prj_content = l_prj_path.parent_path() / g_content;
    for (auto&& i : FSys::recursive_directory_iterator{l_prj_content}) {
      auto l_loc_ = l_loc_content / i.path().lexically_relative(l_prj_content);
      if (i.is_directory()) {
        FSys::create_directories(l_loc_);
      } else {
        if (!FSys::exists(l_loc_) || i.file_size() != FSys::file_size(l_loc_) ||
            i.last_write_time() != FSys::last_write_time(l_loc_)) {
          try {
            FSys::copy_file(i.path(), l_loc_, FSys::copy_options::overwrite_existing);
            FSys::last_write_time(l_loc_, i.last_write_time());
          } catch (const FSys::filesystem_error& error) {
            DOODLE_LOG_ERROR(boost::diagnostic_information(error));
          }
        }
      }
    }
  }
  loc_out_file_path_ = l_loc.parent_path() / arg_.out_file_path;
  if (FSys::exists(loc_out_file_path_)) FSys::remove_all(loc_out_file_path_);
  arg_.ProjectPath = l_loc.lexically_normal().generic_string();

  {
    // 写入数据
    manifest_path_ =
        l_loc.parent_path() / "Saved" / "MovieRenderPipeline" / fmt::format("{}.utxt", core_set::get_set().get_uuid());
    if (!FSys::exists(manifest_path_.parent_path())) FSys::create_directories(manifest_path_.parent_path());
    std::ofstream l_ofs{manifest_path_, std::ios::binary};
    l_ofs << arg_.ManifestValue;
    manifest_path_ = manifest_path_.lexically_normal();
  }
  return true;
}
std::string render_ue4::generate_command_line() const {
  return fmt::format(
      R"({} {} -MoviePipelineConfig="{}")", arg_.ProjectPath, arg_.Args, manifest_path_.generic_string()
  );
}
void render_ue4::run_impl(bool in_r) {
  auto&& l_msg = self_handle_.get_or_emplace<process_message>();
  if (!in_r) {
    l_msg.set_state(process_message::state::fail);
    l_msg.message(fmt::format("project path not exist: {}", arg_.ProjectPath));
    return;
  }
  l_msg.message("开始启动ue4项目文件");
  // 生成命令行
  if (!doodle_lib::Get().ctx().contains<ue_exe_ptr>())
    doodle_lib::Get().ctx().emplace<ue_exe_ptr>() = std::make_shared<ue_exe>();
  doodle_lib::Get().ctx().get<ue_exe_ptr>()->async_run(
      self_handle_, ue_exe::arg_render_queue{generate_command_line()}, [this](auto&&) { end_run(); }
  );
}
void render_ue4::set_meg() {
  auto& l_msg = self_handle_.get_or_emplace<process_message>();
  auto l_prj  = FSys::path{arg_.ProjectPath};
  l_msg.message(fmt::format("开始准备 {}", l_prj));
  l_msg.set_name(l_prj.filename().generic_string());
}
void render_ue4::end_run() {
  boost::asio::post(strand_, [this]() {
    bool l_r;
    try {
      DOODLE_LOG_INFO("开始上传文件 {}", server_file_path);
      l_r = updata_file();
    } catch (const doodle_error& e) {
      l_r = false;
      DOODLE_LOG_ERROR(e);
    } catch (...) {
      l_r = false;
      DOODLE_LOG_ERROR(boost::current_exception_diagnostic_information());
    }
    boost::asio::post(g_io_context(), [this, l_r]() {
      self_handle_.get<process_message>().set_state(
          l_r ? process_message::state::success : process_message::state::fail
      );
      self_handle_.get<process_message>().message(l_r ? "done" : "fail");
      this->send_server_state();
    });
  });
}

bool render_ue4::updata_file() {
  if (!FSys::exists(loc_out_file_path_)) {
    DOODLE_LOG_ERROR("文件不存在 {}", loc_out_file_path_);
    return false;
  }

  if (!FSys::exists(server_file_path)) FSys::create_directories(server_file_path);

  // 上传输出
  for (auto&& i : FSys::recursive_directory_iterator{loc_out_file_path_}) {
    auto l_ser_ = server_file_path / i.path().lexically_relative(loc_out_file_path_);
    if (i.is_directory()) {
      FSys::create_directories(l_ser_);
    } else {
      if (!FSys::exists(l_ser_) || i.file_size() != FSys::file_size(l_ser_) ||
          i.last_write_time() != FSys::last_write_time(l_ser_)) {
        try {
          if (!FSys::exists(l_ser_.parent_path())) FSys::create_directories(l_ser_.parent_path());

          FSys::copy_file(i.path(), l_ser_, FSys::copy_options::overwrite_existing);
          FSys::last_write_time(l_ser_, i.last_write_time());
        } catch (const FSys::filesystem_error& error) {
          DOODLE_LOG_ERROR(boost::diagnostic_information(error));
        }
      }
    }
  }
  return true;
}

void render_ue4::send_server_state() {
  if (self_handle_ && g_ctx().contains<render_farm::work>()) {
    DOODLE_LOG_INFO("开始沟通服务器");
    g_ctx().get<render_farm::work>().send_server_state(self_handle_);
  } else {
    DOODLE_LOG_ERROR("服务器不存在");
  }
}
}  // namespace detail
}  // namespace doodle::render_farm