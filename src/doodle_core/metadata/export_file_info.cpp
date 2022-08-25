//
// Created by TD on 2022/3/24.
//

#include "export_file_info.h"
#include <boost/contract.hpp>
#include <lib_warp/entt_warp.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/logger/logger.h>

namespace doodle {
export_file_info::export_file_info()
    : export_file_info(FSys::path{}, 0, 0, FSys::path{}, export_type::none) {
}
export_file_info::export_file_info(FSys::path in_path, std::int32_t in_start_frame, std::int32_t in_end_frame, FSys::path in_ref_path, export_type in_export_type)
    : file_path(std::move(in_path)),
      start_frame(in_start_frame),
      end_frame(in_end_frame),
      ref_file(std::move(in_ref_path)),
      export_type_(in_export_type),
      upload_path_() {
}

void to_json(nlohmann::json& j, const export_file_info& p) {
  j["file_path"]    = p.file_path;
  j["start_frame"]  = p.start_frame;
  j["end_frame"]    = p.end_frame;
  j["ref_file"]     = p.ref_file;
  j["export_type_"] = p.export_type_;
  j["upload_path"]  = p.upload_path_;
}
void from_json(const nlohmann::json& j, export_file_info& p) {
  j.at("file_path").get_to(p.file_path);
  j.at("start_frame").get_to(p.start_frame);
  j.at("end_frame").get_to(p.end_frame);
  j.at("ref_file").get_to(p.ref_file);
  j.at("export_type_").get_to(p.export_type_);
  if (j.contains("upload_path"))
    j.at("upload_path").get_to(p.upload_path_);
}
void to_json(nlohmann::json& j, const export_file_info::export_type& p) {
  j = magic_enum::enum_name(p);
}
void from_json(const nlohmann::json& j, export_file_info::export_type& p) {
  p = magic_enum::enum_cast<
          export_file_info::export_type>(j.get<std::string>())
          .value_or(export_file_info::export_type::none);
}
void export_file_info::write_file(const entt::handle& in_handle) {
  boost::contract::check l_check =
      boost::contract::function()
          .precondition([&]() {
            in_handle.any_of<export_file_info>() ? void() : throw_exception(doodle_error{"缺失导出组件"s});
          });
  nlohmann::json l_json{};
  entt_tool::save_comm<export_file_info, episodes, shot>(in_handle, l_json);
  auto l_out_path = in_handle.get<export_file_info>().file_path;
  l_out_path.replace_extension(".json_doodle");
  if (exists(l_out_path.parent_path()))
    create_directories(l_out_path.parent_path());

  FSys::ofstream{l_out_path} << l_json.dump();
}
entt::handle export_file_info::read_file(const FSys::path& in_path) {
  boost::contract::check l_check =
      boost::contract::function()
          .precondition([&]() {
            FSys::exists(in_path) ? void() : throw_exception(doodle_error{"文件不存在{}", in_path});
          });

  auto l_json = nlohmann::json::parse(FSys::ifstream{in_path});
  auto l_h    = make_handle();
  entt_tool::load_comm<export_file_info, episodes, shot>(l_h, l_json);
  return l_h;
}
}  // namespace doodle
