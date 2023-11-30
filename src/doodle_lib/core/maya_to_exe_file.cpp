//
// Created by TD on 2023/11/17.
//

#include "maya_to_exe_file.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/file_association.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/thread_pool/process_message.h>

#include <doodle_lib/exe_warp/ue_exe.h>

#include <boost/asio.hpp>
namespace doodle {

namespace maya_to_exe_file_ns {
struct maya_out_arg {
  FSys::path out_file{};
  // 引用文件
  FSys::path ref_file{};
  friend void from_json(const nlohmann::json &nlohmann_json_j, maya_out_arg &nlohmann_json_t) {
    nlohmann_json_j["out_file"].get_to(nlohmann_json_t.out_file);
    nlohmann_json_j["ref_file"].get_to(nlohmann_json_t.ref_file);
  };
};
struct out_file_export {
  std::string type_;
  FSys::path path_;
  friend void to_json(nlohmann::json &j, const out_file_export &p) {
    j["type"] = p.type_;
    j["path"] = p.path_.generic_string();
  }
};
struct out_file {
  std::string project;
  std::int32_t begin_time;
  std::int32_t end_time;
  std::int32_t episode;
  std::int32_t shot;
  std::string shot_ab;
  FSys::path out_file_dir;
  std::string main_map;
  std::vector<out_file_export> files;
  friend void to_json(nlohmann::json &j, const out_file &p) {
    j["project"]      = p.project;
    j["begin_time"]   = p.begin_time;
    j["end_time"]     = p.end_time;
    j["episode"]      = p.episode;
    j["shot"]         = p.shot;
    j["shot_ab"]      = p.shot_ab;
    j["out_file_dir"] = p.out_file_dir.generic_string();
    j["main_map"]     = p.main_map;
    j["files"]        = p.files;
  }
};
}  // namespace maya_to_exe_file_ns

FSys::path maya_to_exe_file::write_python_script() const { return "D:/test"; }

FSys::path maya_to_exe_file::gen_render_config_file() const {
  auto l_maya_out_arg =
      nlohmann::json ::parse(data_->maya_out_data_).get<std::vector<maya_to_exe_file_ns::maya_out_arg>>();
  l_maya_out_arg = l_maya_out_arg | ranges::views::filter([](const maya_to_exe_file_ns::maya_out_arg &in_arg) {
                     return !in_arg.out_file.empty() && FSys::exists(in_arg.out_file);
                   }) |
                   ranges::to_vector;

  auto l_path = l_maya_out_arg[0].out_file.filename();

  maya_to_exe_file_ns::out_file l_out_file{};
  {
    episodes l_eps{};
    if (l_eps.analysis(l_path)) {
      l_out_file.episode = l_eps.p_episodes;
    }
  }
  {
    shot l_shot{};
    if (l_shot.analysis(l_path)) {
      l_out_file.shot = l_shot.p_shot;
      if (l_shot.p_shot_enum != shot::shot_ab_enum::None) l_out_file.shot_ab = l_shot.p_shot_ab;
    }
  }

  auto l_str = l_path.generic_string();
  {
    std::regex const l_regex{R"(_(\d+)-(\d+))"};
    if (std::smatch l_match{}; std::regex_search(l_str, l_match, l_regex)) {
      l_out_file.begin_time = std::stoi(l_match[1].str());
      l_out_file.end_time   = std::stoi(l_match[2].str());
    }
  }
  l_out_file.project      = l_str.substr(0, l_str.find_first_of('_'));
  l_out_file.out_file_dir = data_->render_project_ / g_saved / g_movie_renders /
                            fmt::format("Ep_{}_sc_{}{}", l_out_file.episode, l_out_file.shot, l_out_file.shot_ab);
  data_->out_dir = l_out_file.out_file_dir;

  l_out_file.files =
      l_maya_out_arg | ranges::views::transform([](const maya_to_exe_file_ns::maya_out_arg &in_arg) {
        return maya_to_exe_file_ns::out_file_export{
            in_arg.out_file.filename().generic_string().find("_camera_") != std::string::npos ? "cam" : "char",
            in_arg.out_file};
      }) |
      ranges::to<std::vector<maya_to_exe_file_ns::out_file_export>>();
  l_out_file.main_map         = data_->render_map_;
  nlohmann::json const l_json = l_out_file;
  return FSys::write_tmp_file("render_ue", l_json.dump(), ".json");
}
void maya_to_exe_file::begin_render(boost::system::error_code in_error_code) const {
  if (!maya_out_file_.empty() && FSys::exists(maya_out_file_)) {
    std::ifstream l_file{maya_out_file_};
    data_->maya_out_data_ = {std::istreambuf_iterator<char>{l_file}, std::istreambuf_iterator<char>{}};
  }

  auto &l_msg = msg_.get<process_message>();
  l_msg.set_state(l_msg.run);
  l_msg.message("开始处理 maya 输出文件");
  if (data_->maya_out_data_.empty()) {
    l_msg.message("maya结束进程后未能成功输出文件");
    in_error_code.assign(error_enum::file_not_exists, doodle_category::get());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    data_->end_call_(in_error_code);
    return;
  }
  auto l_maya_out_arg =
      nlohmann::json ::parse(data_->maya_out_data_).get<std::vector<maya_to_exe_file_ns::maya_out_arg>>();

  if (l_maya_out_arg.empty()) {
    auto &l_msg = msg_.get<process_message>();
    l_msg.message("maya结束进程后未能成功输出文件");
    in_error_code.assign(error_enum::file_not_exists, doodle_category::get());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    data_->end_call_(in_error_code);
    return;
  }

  auto l_id_map_tmp = g_reg()->view<database, assets_file, file_association_ref>();
  auto l_id_map     = l_id_map_tmp | ranges::views::transform([](const entt::entity &in_entity) {
                    return std::make_pair(g_reg()->get<database>(in_entity).uuid(), in_entity);
                  }) |
                  ranges::to<std::unordered_map<uuid, entt::entity>>();

  auto l_refs =
      l_maya_out_arg |
      ranges::views::transform([](const maya_to_exe_file_ns::maya_out_arg &in_arg) { return in_arg.ref_file; }) |
      ranges::views::filter([](const FSys::path &in_arg) { return FSys::exists(in_arg); }) |
      ranges::views::transform([&](const FSys::path &in_arg) -> entt::handle {
        auto l_uuid = FSys::software_flag_file(in_arg);
        log_info(fmt::format("maya_to_exe_file get uuid :{}", l_uuid));

        if (l_id_map.contains(l_uuid)) {
          return entt::handle{*g_reg(), l_id_map.at(l_uuid)};
        }
        l_msg.message(fmt::format("文件 {} 找不到引用, 继续输出将变为不正确排屏", in_arg));
        return entt::handle{};
      }) |
      ranges::to<std::vector<entt::handle>>();

  if (!ranges::all_of(l_refs, [&](const entt::handle &in_handle) -> bool {
        if (!in_handle) {
          return false;
        }
        const auto &l_file = in_handle.get<assets_file>().path_attr();
        if (!in_handle.get<file_association_ref>()) {
          l_msg.message(fmt::format("未查找到文件 {} 的引用", l_file));
          return false;
        }
        if (!in_handle.get<file_association_ref>().get<file_association>().ue_file) {
          l_msg.message(fmt::format("未查找到文件 {} 的 ue 引用", l_file));
          return false;
        }
        if (!in_handle.get<file_association_ref>().get<file_association>().ue_file.all_of<assets_file>()) {
          l_msg.message(fmt::format("文件 {} 的 ue 引用无效", l_file));
          return false;
        }
        return true;
      })) {
    auto &l_msg   = msg_.get<process_message>();
    in_error_code.assign(error_enum::file_not_exists, doodle_category::get());
    l_msg.message("maya结束进程后 输出文件引用查找有误");
    data_->end_call_(in_error_code);
    BOOST_ASIO_ERROR_LOCATION(in_error_code);

    return;
  }
  if (!ranges::any_of(l_refs, [&](const entt::handle &in_handle) {
        return in_handle.get<file_association_ref>().get<file_association>().ue_file.all_of<ue_main_map>();
      })) {
    auto &l_msg = msg_.get<process_message>();
    l_msg.message("未查找到主项目文件");
    data_->end_call_(in_error_code);
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    return;
  }

  // sort ue_main_map
  l_refs |= ranges::actions::sort([](const entt::handle &in_r, const entt::handle &in_l) {
    return in_r.get<file_association_ref>().get<file_association>().ue_file.all_of<ue_main_map>() &&
           !in_l.get<file_association_ref>().get<file_association>().ue_file.all_of<ue_main_map>();
  });

  for (auto &&h : l_refs) {
    auto l_is_se = h.get<file_association_ref>().get<file_association>().ue_file.all_of<ue_main_map>();
    down_file(h.get<file_association_ref>().get<file_association>().ue_file.get<assets_file>().path_attr(), l_is_se);
    if (l_is_se)
      data_->render_map_ = h.get<file_association_ref>().get<file_association>().ue_file.get<ue_main_map>().map_path_;
  }
  if (!g_ctx().contains<ue_exe_ptr>()) g_ctx().emplace<ue_exe_ptr>() = std::make_shared<ue_exe>();

  render();
}

void maya_to_exe_file::operator()(boost::system::error_code in_error_code) const {
  if (!data_->logger_) {
    default_logger_raw()->log(log_loc(), level::level_enum::err, "缺失组建错误 缺失日志组件");
    in_error_code.assign(error_enum::component_missing_error, doodle_category::get());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    return;
  }
  if (in_error_code) {
    log_error(fmt::format("maya_to_exe_file error:{}", in_error_code));
    data_->end_call_(in_error_code);
    return;
  }
  switch (data_->render_type_) {
    case render_type::render:
      begin_render(in_error_code);
      break;
    case render_type::update:
      data_->render_type_ = render_type::update_end;
      update_file(in_error_code);
      break;
    case render_type::update_end:
      break;
  }
}

void maya_to_exe_file::down_file(const FSys::path &in_path, bool is_scene) const {
  static auto g_root{FSys::path{"D:/doodle/cache/ue"}};

  if (is_scene) {
    data_->render_project_ = g_root / in_path.stem();
    if (!FSys::exists(data_->render_project_)) FSys::create_directories(data_->render_project_);
  }

  auto l_loc_prj = data_->render_project_ / g_content;
  auto l_rem_prj = in_path / g_content;

  // 复制内容文件夹
  for (auto &&l_file : FSys::recursive_directory_iterator{l_rem_prj}) {
    auto l_loc_file = l_loc_prj / l_file.path().lexically_relative(l_rem_prj);
    if (l_file.is_directory())
      FSys::create_directories(l_loc_file);
    else {
      if (!FSys::exists(l_loc_file) || l_file.file_size() != FSys::file_size(l_loc_file) ||
          l_file.last_write_time() != FSys::last_write_time(l_loc_file)) {
        try {
          FSys::copy_file(l_file.path(), l_loc_file, FSys::copy_options::overwrite_existing);
          FSys::last_write_time(l_loc_file, l_file.last_write_time());
        } catch (const FSys::filesystem_error &error) {
          DOODLE_LOG_ERROR(boost::diagnostic_information(error));
        }
      }
    }
  }
  if (!is_scene) return;

  // 复制配置文件夹
  auto l_loc_config = data_->render_project_ / g_config;
  auto l_rem_config = in_path / g_config;
  FSys::copy(l_rem_config, l_loc_config, FSys::copy_options::overwrite_existing | FSys::copy_options::recursive);
  // 复制项目文件
  for (auto &&l_file : FSys::directory_iterator{in_path}) {
    auto l_loc_file = data_->render_project_ / l_file.path().filename();
    if (l_file.path().extension() == g_uproject) {
      FSys::copy(l_file.path(), l_loc_file, FSys::copy_options::overwrite_existing);
      data_->render_project_file_ = l_loc_file;
      break;
    }
  }
}
void maya_to_exe_file::render() const {
  auto &l_msg = msg_.get<process_message>();
  auto l_exe  = g_ctx().get<ue_exe_ptr>();
  auto l_path = gen_render_config_file();
  l_exe->async_run(
      msg_,
      ue_exe_ptr::element_type ::arg_render_queue{
          fmt::format("{} -ExecutePythonScript={} {}", data_->render_project_file_, write_python_script(), l_path)},
      *this
  );
}

void maya_to_exe_file::update_file(boost::system::error_code in_error_code) const {
  auto &l_msg = msg_.get<process_message>();
  l_msg.message("排屏完成, 开始上传文件");

  if (!FSys::exists(update_dir_)) {
    FSys::create_directories(update_dir_);
  }

  auto l_loc_prj = data_->render_project_ / g_content;
  auto l_rem_prj = update_dir_ / g_content;

  // 复制内容文件夹
  for (auto &&l_file : FSys::recursive_directory_iterator{l_loc_prj}) {
    auto l_rem_file = l_rem_prj / l_file.path().lexically_relative(l_loc_prj);
    if (l_file.is_directory())
      FSys::create_directories(l_rem_file);
    else {
      if (!FSys::exists(l_rem_file) || l_file.file_size() != FSys::file_size(l_rem_file) ||
          l_file.last_write_time() != FSys::last_write_time(l_rem_file)) {
        try {
          FSys::copy_file(l_file.path(), l_rem_file, FSys::copy_options::overwrite_existing);
          FSys::last_write_time(l_rem_file, l_file.last_write_time());
        } catch (const FSys::filesystem_error &error) {
          DOODLE_LOG_ERROR(boost::diagnostic_information(error));
        }
      }
    }
  }

  // 复制输出的文件
  auto l_out_loc_dir = data_->out_dir;
  if (!FSys::exists(l_out_loc_dir)) {
    l_msg.message(fmt::format("输出文件夹 {} 不存在", l_out_loc_dir));
    boost::system::error_code l_error_code{error_enum::file_not_exists};
    BOOST_ASIO_ERROR_LOCATION(l_error_code);
    data_->end_call_(l_error_code);
    return;
  }

  auto l_out_rem_dir = update_dir_ / g_saved / g_movie_renders / l_out_loc_dir.filename();
  if (!FSys::exists(l_out_rem_dir)) FSys::create_directories(l_out_rem_dir);
  for (auto &&l_file : FSys::directory_iterator{l_out_loc_dir}) {
    auto l_rem_file = l_out_rem_dir / l_file.path().filename();
    if (!FSys::exists(l_rem_file) || l_file.file_size() != FSys::file_size(l_rem_file) ||
        l_file.last_write_time() != FSys::last_write_time(l_rem_file)) {
      try {
        FSys::copy_file(l_file.path(), l_rem_file, FSys::copy_options::overwrite_existing);
        FSys::last_write_time(l_rem_file, l_file.last_write_time());
      } catch (const FSys::filesystem_error &error) {
        DOODLE_LOG_ERROR(boost::diagnostic_information(error));
      }
    }
  }

  data_->end_call_(in_error_code);
}

}  // namespace doodle