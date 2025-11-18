#include "qcloth_arg.h"

namespace doodle {
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

void from_json(const nlohmann::json& in_json, qcloth_update_arg& out_obj) {
  if (in_json.contains("alembic_files")) in_json.at("alembic_files").get_to(out_obj.alembic_files_);
  if (in_json.contains("task_id")) in_json.at("task_id").get_to(out_obj.task_id_);
}

void to_json(nlohmann::json& in_json, const qcloth_update_arg& out_obj) {
  in_json["alembic_files"] = out_obj.alembic_files_;
  in_json["task_id"]       = out_obj.task_id_;
}

boost::asio::awaitable<void> qcloth_arg::run() { return arg::async_run_maya(); }

boost::asio::awaitable<void> qcloth_update_arg::run() {
  SPDLOG_INFO("上传qcloth abc文件数量 {} {}", alembic_files_.size(), fmt::join(alembic_files_, "\n "));
  for (auto& p : alembic_files_) {
    co_await kitsu_client_->upload_shot_animation_export_file(task_id_, p.parent_path(), p.filename());
  }
  co_return;
}

}  // namespace doodle