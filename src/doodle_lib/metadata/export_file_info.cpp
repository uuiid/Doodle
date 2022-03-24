//
// Created by TD on 2022/3/24.
//

#include "export_file_info.h"
#include <boost/contract.hpp>
#include <doodle_lib/lib_warp/entt_warp.h>
#include <metadata/shot.h>
#include <metadata/episodes.h>
//#include <doodle_lib/core

namespace doodle {
export_file_info::export_file_info()
    : export_file_info(FSys::path{}, 0, 0, FSys::path{}) {
}
export_file_info::export_file_info(FSys::path in_path,
                                   std::int32_t in_start_frame,
                                   std::int32_t in_end_frame,
                                   FSys::path in_ref_path)
    : file_path(std::move(in_path)),
      start_frame(in_start_frame),
      end_frame(in_end_frame),
      ref_file(std::move(in_ref_path)) {
}

void to_json(nlohmann::json& j, const export_file_info& p) {
  j["file_path"]   = p.file_path;
  j["start_frame"] = p.start_frame;
  j["end_frame"]   = p.end_frame;
  j["ref_file"]    = p.ref_file;
}
void from_json(const nlohmann::json& j, export_file_info& p) {
  j.at("file_path").get_to(p.file_path);
  j.at("start_frame").get_to(p.start_frame);
  j.at("end_frame").get_to(p.end_frame);
  j.at("ref_file").get_to(p.ref_file);
}
void export_file_info::write_file(const entt::handle& in_handle) {
  boost::contract::check l_check =
      boost::contract::function()
          .precondition([&]() {
            chick_true<doodle_error>(in_handle.any_of<export_file_info>(),
                                     DOODLE_LOC,
                                     "缺失导出组件");
          });
  nlohmann::json l_json{};
  entt_tool::save_comm<export_file_info, episodes, shot>(in_handle, l_json);
  auto l_out_path = in_handle.get<export_file_info>().file_path;
  l_out_path.replace_extension(".json_doodle");
  if (exists(l_out_path.parent_path()))
    create_directories(l_out_path.parent_path());

  FSys::ofstream{l_out_path} << l_json.dump();
}
}  // namespace doodle
