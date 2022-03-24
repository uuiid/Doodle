//
// Created by TD on 2022/3/24.
//

#include "export_file_info.h"
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
  j["ref_file"]   = p.ref_file;
}
void from_json(const nlohmann::json& j, export_file_info& p) {
  j.at("file_path").get_to(p.file_path);
  j.at("start_frame").get_to(p.start_frame);
  j.at("end_frame").get_to(p.end_frame);
  j.at("ref_file").get_to(p.ref_file);
}
}  // namespace doodle
