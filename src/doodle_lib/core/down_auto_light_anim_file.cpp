//
// Created by TD on 2024/1/5.
//

#include "down_auto_light_anim_file.h"

#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/file_association.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/core/thread_copy_io.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/ue_exe.h>

#include <boost/beast.hpp>
namespace doodle {

void down_auto_light_anim_file::init() {
  data_->logger_ = msg_.get<process_message>().logger();

  if (!g_ctx().contains<ue_exe_ptr>()) g_ctx().emplace<ue_exe_ptr>(std::make_shared<ue_exe>());
}

std::vector<down_auto_light_anim_file::association_data> down_auto_light_anim_file::fetch_association_data(
    const std::vector<boost::uuids::uuid> &in_uuid, boost::system::error_code &out_error_code
) const {
  std::vector<down_auto_light_anim_file::association_data> l_out{};
  boost::beast::tcp_stream l_stream{g_io_context()};

  try {
    l_stream.connect(boost::asio::ip::tcp::endpoint{boost::asio::ip::address_v4::from_string("192.168.40.181"), 50026});
    for (auto &&i : in_uuid) {
      boost::beast::http::request<boost::beast::http::empty_body> l_req{
          boost::beast::http::verb::get, fmt::format("api/doodle/file_association/{}", i), 11
      };
      l_req.set(boost::beast::http::field::host, "192.168.40.181:50026");
      l_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
      l_req.set(boost::beast::http::field::accept, "application/json");
      l_req.prepare_payload();
      boost::beast::http::write(l_stream, l_req);
      boost::beast::flat_buffer l_buffer{};
      boost::beast::http::response<boost::beast::http::string_body> l_res;
      boost::beast::http::read(l_stream, l_buffer, l_res);
      if (l_res.result() != boost::beast::http::status::ok) {
        if (l_res.result() == boost::beast::http::status::not_found) {
          data_->logger_->log(log_loc(), level::err, "未找到关联数据:{}", i);
          out_error_code = boost::system::error_code{
              boost::system::errc::no_such_file_or_directory, boost::system::generic_category()
          };
          return l_out;
        }
        continue;
      }

      auto l_json = nlohmann::json::parse(l_res.body());

      association_data l_data{
          .id_        = i,
          .maya_file_ = l_json.at("maya_file").get<std::string>(),
          .ue_file_   = l_json.at("ue_file").get<std::string>(),
          .type_      = l_json.at("type").get<details::assets_type_enum>(),
      };
      l_out.emplace_back(std::move(l_data));
    }

  } catch (const std::exception &e) {
    data_->logger_->log(log_loc(), level::err, "连接服务器失败:{}", e.what());
    out_error_code =
        boost::system::error_code{boost::system::errc::connection_refused, boost::system::generic_category()};
  }
  for (auto &&i : l_out) {
    i.ue_prj_path_ = ue_main_map::find_ue_project_file(i.ue_file_);
  }
  return l_out;
}

void down_auto_light_anim_file::analysis_out_file(boost::system::error_code in_error_code) const {
  // 检查项目
  if (!msg_.all_of<project>()) {
    data_->logger_->log(log_loc(), level::err, "没有项目组件, 失败");
    in_error_code.assign(error_enum::component_missing_error, doodle_category::get());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  project l_project = msg_.get<project>();
  std::vector<boost::uuids::uuid> l_refs_tmp{};

  for (auto &&i : data_->out_maya_arg_.out_file_list) {
    if (!FSys::exists(i.ref_file)) continue;
    auto l_uuid = FSys::software_flag_file(i.ref_file);
    if (l_uuid.is_nil()) continue;

    l_refs_tmp.emplace_back(std::move(l_uuid));
  }

  l_refs_tmp |= ranges::actions::unique;

  auto l_refs = fetch_association_data(l_refs_tmp, in_error_code);
  if (in_error_code) {
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }

  // 检查文件

  auto l_scene_uuid = boost::uuids::nil_uuid();
  FSys::path l_down_path_file_name{};

  for (auto &&h : l_refs) {
    if (auto l_is_e = h.ue_file_.empty(), l_is_ex = FSys::exists(h.ue_file_); l_is_e || !l_is_ex) {
      if (l_is_e)
        data_->logger_->log(log_loc(), level::level_enum::err, "文件 {} 的 ue 引用无效, 为空", h.maya_file_);
      else if (!l_is_ex)
        data_->logger_->log(log_loc(), level::level_enum::err, "文件 {} 的 ue {} 引用不存在", h.maya_file_, h.ue_file_);

      in_error_code.assign(error_enum::file_not_exists, doodle_category::get());
      BOOST_ASIO_ERROR_LOCATION(in_error_code);
      wait_op_->ec_ = in_error_code;
      wait_op_->complete();
      return;
    }

    if (h.type_ == details::assets_type_enum::scene) {
      l_scene_uuid          = h.id_;
      l_down_path_file_name = h.ue_prj_path_.parent_path().filename();
    }
  }
  if (l_scene_uuid.is_nil()) {
    data_->logger_->log(log_loc(), level::level_enum::err, "未查找到主项目文件(没有找到场景文件)");
    in_error_code.assign(error_enum::file_not_exists, doodle_category::get());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }

  static auto g_root{FSys::path{"D:/doodle/cache/ue"}};
  std::vector<std::pair<FSys::path, FSys::path>> l_copy_path{};

  for (auto &&h : l_refs) {
    auto l_down_path  = h.ue_prj_path_.parent_path();
    auto l_root       = h.ue_prj_path_.parent_path() / doodle_config::ue4_content;
    auto l_local_path = g_root / l_project.p_shor_str / l_down_path_file_name;

    switch (h.type_) {
      // 场景文件
      case details::assets_type_enum::scene: {
        auto l_original = h.ue_file_.lexically_relative(l_root);
        data_->down_info_.scene_file_ =
            fmt::format("/Game/{}/{}", l_original.parent_path().generic_string(), l_original.stem());

        l_copy_path.emplace_back(l_down_path / doodle_config::ue4_content, l_local_path / doodle_config::ue4_content);
        // 配置文件夹复制
        l_copy_path.emplace_back(l_down_path / doodle_config::ue4_config, l_local_path / doodle_config::ue4_config);
        // 复制项目文件
        if (!FSys::exists(l_local_path / h.ue_prj_path_.filename()))
          l_copy_path.emplace_back(h.ue_prj_path_, l_local_path / h.ue_prj_path_.filename());
        data_->down_info_.render_project_ = l_local_path / h.ue_prj_path_.filename();
      } break;

      // 角色文件
      case details::assets_type_enum::character: {
        l_copy_path.emplace_back(l_down_path / doodle_config::ue4_content, l_local_path / doodle_config::ue4_content);
      } break;

      // 道具文件
      case details::assets_type_enum::prop: {
        auto l_prop_path = h.ue_file_.lexically_relative(l_root / doodle_config::ue4_content / "Prop");
        if (l_prop_path.empty()) continue;
        auto l_prop_path_name = *l_prop_path.begin();
        l_copy_path.emplace_back(
            l_down_path / doodle_config::ue4_content / "Prop" / l_prop_path_name,
            l_local_path / doodle_config::ue4_content / "Prop" / l_prop_path_name
        );
      } break;

      default:
        break;
    }
  }
  g_ctx().get<ue_exe_ptr>()->async_copy_old_project(
      msg_, l_copy_path, boost::asio::bind_executor(g_io_context(), *this)
  );
}

void down_auto_light_anim_file::operator()(
    boost::system::error_code in_error_code, const maya_exe_ns::maya_out_arg &in_vector
) const {
  if (!data_->logger_) {
    default_logger_raw()->log(log_loc(), level::level_enum::err, "缺失组建错误 缺失日志组件");
    in_error_code.assign(error_enum::component_missing_error, doodle_category::get());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  if (in_error_code) {
    data_->logger_->log(log_loc(), level::level_enum::err, "maya_to_exe_file error:{}", in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  data_->out_maya_arg_ = in_vector;
  analysis_out_file(in_error_code);
}
void down_auto_light_anim_file::operator()(boost::system::error_code in_error_code) const {
  if (in_error_code) {
    data_->logger_->log(log_loc(), level::level_enum::err, "maya_to_exe_file error:{}", in_error_code);
    wait_op_->ec_ = in_error_code;
    wait_op_->complete();
    return;
  }
  set_info_(data_->down_info_);
  wait_op_->ec_ = in_error_code;
  wait_op_->complete();
}

}  // namespace doodle