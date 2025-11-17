#include "export_fbx_arg.h"

namespace doodle {
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
boost::asio::awaitable<void> export_fbx_arg::run() { co_await arg::async_run_maya(); }

}  // namespace doodle