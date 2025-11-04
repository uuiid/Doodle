#include "inspect_maya.h"

#include "doodle_core/exception/exception.h"

namespace doodle {

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
boost::asio::awaitable<void> inspect_file_arg::run() {
  if (!client_) throw_exception(doodle_error{"客户端指针未初始化"});
  if (task_id_.is_nil()) throw_exception(doodle_error{"任务ID未初始化"});
  co_await arg::async_run_maya();

  co_await client_->upload_asset_file_maya(task_id_, file_path);
  co_return;
}
}  // namespace doodle