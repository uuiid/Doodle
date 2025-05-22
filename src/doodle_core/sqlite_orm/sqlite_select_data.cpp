//
// Created by TD on 25-4-15.
//

#include "sqlite_select_data.h"
namespace doodle {

void to_json(nlohmann::json& j, const todo_t::comment_t& p) {
  j["text"]      = p.text_;
  j["date"]      = p.date_;
  j["person_id"] = p.person_id_;
}

void to_json(nlohmann::json& j, const todo_t& p) {
  j["id"]                     = p.uuid_id_;
  j["name"]                   = p.name_;
  j["description"]            = p.description_;
  j["priority"]               = p.priority_;
  j["difficulty"]             = p.difficulty_;
  j["duration"]               = p.duration_;
  j["estimation"]             = p.estimation_;
  j["completion_rate"]        = p.completion_rate_;
  j["retake_count"]           = p.retake_count_;
  j["sort_order"]             = p.sort_order_;
  j["start_date"]             = p.start_date_;
  j["due_date"]               = p.due_date_;
  j["real_start_date"]        = p.real_start_date_;
  j["end_date"]               = p.end_date_;
  j["done_date"]              = p.done_date_;
  j["last_comment_date"]      = p.last_comment_date_;
  j["nb_assets_ready"]        = p.nb_assets_ready_;
  j["data"]                   = p.data_;
  j["shotgun_id"]             = p.shotgun_id_;
  j["last_preview_file_id"]   = p.last_preview_file_id_;
  j["task_type_id"]           = p.task_type_id_;
  j["task_status_id"]         = p.task_status_id_;
  j["assigner_id"]            = p.assigner_id_;
  j["assignees"]              = p.assignees_;
  j["created_at"]             = p.created_at_;
  j["updated_at"]             = p.updated_at_;

  j["project_name"]           = p.project_name_;
  j["project_id"]             = p.project_id_;
  j["project_has_avatar"]     = p.project_has_avatar_;

  j["entity_id"]              = p.entity_uuid_id_;
  j["entity_name"]            = p.entity_name_;
  j["entity_description"]     = p.entity_description_;
  j["entity_data"]            = p.entity_data_;
  j["entity_preview_file_id"] = p.entity_preview_file_id_;
  j["entity_source_id"]       = p.entity_source_id_;
  j["entity_type_name"]       = p.asset_type_name_;
  j["entity_canceled"]        = p.entity_canceled_;

  j["sequence_name"]          = nlohmann::json::value_t::null;
  j["episode_id"]             = "";
  j["episode_name"]           = nlohmann::json::value_t::null;

  j["task_estimation"]        = p.estimation_;
  j["task_duration"]          = p.duration_;
  j["task_start_date"]        = p.start_date_;
  j["task_due_date"]          = p.due_date_;

  j["task_type_name"]         = p.task_type_name_;
  j["task_type_for_entity"]   = p.task_type_for_entity_;
  j["task_status_name"]       = p.task_status_name_;
  j["task_type_color"]        = p.task_type_color_;
  j["task_status_color"]      = p.task_status_color_;
  j["task_status_short_name"] = p.task_status_short_name_;
  if (p.last_comment_)
    j["last_comment"] = p.last_comment_;
  else
    j["last_comment"] = nlohmann::json::value_t::object;
}

void to_json(nlohmann::json& j, const project_and_status_t& p) {
  j["id"]                                 = p.uuid_id_;
  j["name"]                               = p.name_;
  j["code"]                               = p.code_;
  j["description"]                        = p.description_;
  j["shotgun_id"]                         = p.shotgun_id_;
  j["file_tree"]                          = p.file_tree_;
  j["data"]                               = p.data_;
  j["has_avatar"]                         = p.has_avatar_;
  j["fps"]                                = p.fps_;
  j["ratio"]                              = p.ratio_;
  j["resolution"]                         = p.resolution_;
  j["production_type"]                    = p.production_type_;
  j["production_style"]                   = p.production_style_;
  j["start_date"]                         = p.start_date_;
  j["end_date"]                           = p.end_date_;
  j["man_days"]                           = p.man_days_;
  j["nb_episodes"]                        = p.nb_episodes_;
  j["episode_span"]                       = p.episode_span_;
  j["max_retakes"]                        = p.max_retakes_;
  j["is_clients_isolated"]                = p.is_clients_isolated_;
  j["is_preview_download_allowed"]        = p.is_preview_download_allowed_;
  j["is_set_preview_automated"]           = p.is_set_preview_automated_;
  j["homepage"]                           = p.homepage_;
  j["is_publish_default_for_artists"]     = p.is_publish_default_for_artists_;
  j["hd_bitrate_compression"]             = p.hd_bitrate_compression_;
  j["ld_bitrate_compression"]             = p.ld_bitrate_compression_;
  j["project_status_id"]                  = p.project_status_id_;
  j["default_preview_background_file_id"] = p.default_preview_background_file_id_;
  j["project_status_name"]                = p.project_status_name_;
}

void to_json(nlohmann::json& j, const get_comments_t::person_t& p) {
  j["id"]         = p.uuid_id_;
  j["first_name"] = p.first_name_;
  j["last_name"]  = p.last_name_;
  j["has_avatar"] = p.has_avatar_;
}

void to_json(nlohmann::json& j, const get_comments_t::task_status_t& p) {
  j["id"]         = p.uuid_id_;
  j["name"]       = p.name_;
  j["color"]      = p.color_;
  j["short_name"] = p.short_name_;
}
void to_json(nlohmann::json& j, const get_comments_t::previews_t& p) {
  j["id"]                = p.uuid_id_;
  j["task_id"]           = p.task_id_;
  j["revision"]          = p.revision_;
  j["extension"]         = p.extension_;
  j["width"]             = p.width_;
  j["height"]            = p.height_;
  j["duration"]          = p.duration_;
  j["status"]            = p.status_;
  j["validation_status"] = p.validation_status_;
  j["original_name"]     = p.original_name_;
  j["position"]          = p.position_;
  j["annotations"]       = p.annotations_;
}
void to_json(nlohmann::json& j, const get_comments_t::attachment_files_t& p) {
  j["id"]        = p.uuid_id_;
  j["name"]      = p.name_;
  j["extension"] = p.extension_;
  j["size"]      = p.size_;
}
void to_json(nlohmann::json& j, const get_comments_t& p) {
  j["id"]                  = p.uuid_id_;
  j["shotgun_id"]          = p.shotgun_id_;
  j["object_id"]           = p.object_id_;
  j["object_type"]         = p.object_type_;
  j["text"]                = p.text_;
  j["data"]                = p.data_;
  j["replies"]             = p.replies_;
  j["checklist"]           = p.checklist_;
  j["pinned"]              = p.pinned_;
  j["links"]               = p.links_;
  j["created_at"]          = p.created_at_;
  j["updated_at"]          = p.updated_at_;
  j["task_status_id"]      = p.task_status_id_;
  j["person_id"]           = p.person_id_;
  j["editor_id"]           = p.editor_id_;
  j["preview_file_id"]     = p.preview_file_id_;
  j["acknowledgements"]    = p.acknowledgements_;
  j["mentions"]            = p.mentions_;
  j["department_mentions"] = p.department_mentions_;
  j["persons"]             = p.persons_;
  j["editors"]             = p.editors_;
  j["task_status"]         = p.task_statuses_;
  j["previews"]            = p.previews_;
  j["attachment_files"]    = p.attachment_files_;
}
void to_json(nlohmann::json& j, const assets_and_tasks_t::task_t& p) {
  j["id"]                = p.uuid_id_;
  j["due_date"]          = p.due_date_;
  j["done_date"]         = p.done_date_;
  j["duration"]          = p.duration_;
  j["entity_id"]         = p.entity_id_;
  j["estimation"]        = p.estimation_;
  j["end_date"]          = p.end_date_;
  j["last_comment_date"] = p.last_comment_date_;
  j["last_preview_file"] = p.last_preview_file_id_;
  j["priority"]          = p.priority_;
  j["real_start_date"]   = p.real_start_date_;
  j["retake_count"]      = p.retake_count_;
  j["start_date"]        = p.start_date_;
  j["difficulty"]        = p.difficulty_;
  j["task_type_id"]      = p.task_type_id_;
  j["task_status_id"]    = p.task_status_id_;
  j["assignees"]         = p.assigner_ids_;
  j["is_subscribed"]     = p.is_subscribed_;
  j["path"]              = "";
}

void to_json(nlohmann::json& j, const assets_and_tasks_t& p) {
  j["id"]                  = p.uuid_id_;
  j["name"]                = p.name_;
  j["preview_file_id"]     = p.preview_file_id_;
  j["description"]         = p.description_;
  j["asset_type_name"]     = p.asset_type_name_;
  j["asset_type_id"]       = p.asset_type_id_;
  j["canceled"]            = p.canceled_;
  j["ready_for"]           = p.ready_for_;
  j["source_id"]           = p.source_id_;
  j["is_casting_standby"]  = p.is_casting_standby_;
  j["is_shared"]           = p.is_shared_;
  j["data"]                = p.data_;
  j["casting_episode_ids"] = p.casting_episode_ids_;
  j["tasks"]               = p.tasks_;
}

void to_json(nlohmann::json& j, const entities_and_tasks_t::task_t& p) {
  j["id"]                = p.uuid_id_;
  j["estimation"]        = p.estimation_;
  j["entity_id"]         = p.entity_id_;
  j["end_date"]          = p.end_date_;
  j["due_date"]          = p.due_date_;
  j["done_date"]         = p.done_date_;
  j["duration"]          = p.duration_;
  j["is_subscribed"]     = p.is_subscribed_;
  j["last_comment_date"] = p.last_comment_date_;
  j["last_preview_file"] = p.last_preview_file_id_;
  j["priority"]          = p.priority_;
  j["real_start_date"]   = p.real_start_date_;
  j["retake_count"]      = p.retake_count_;
  j["start_date"]        = p.start_date_;
  j["difficulty"]        = p.difficulty_;
  j["task_type_id"]      = p.task_type_id_;
  j["task_status_id"]    = p.task_status_id_;
  j["assignees"]         = p.assigners_;
}

void to_json(nlohmann::json& j, const entities_and_tasks_t& p) {
  j["id"]              = p.uuid_id_;
  j["name"]            = p.name_;
  j["status"]          = p.status_;
  j["episode_id"]      = p.episode_id_;
  j["description"]     = p.description_;

  j["frame_in"]        = p.frame_in_ ? nlohmann::json{} = p.frame_in_ : nlohmann::json{};
  j["frame_out"]       = p.frame_out_ ? nlohmann::json{} = p.frame_out_ : nlohmann::json{};
  j["fps"]             = p.fps_ ? nlohmann::json{} = p.fps_ : nlohmann::json{};
  j["preview_file_id"] = p.preview_file_id_;
  j["canceled"]        = p.canceled_;
  j["data"]            = p.data_;
  j["tasks"]           = p.tasks_;
}
void to_json(nlohmann::json& j, const preview_files_for_entity_t& p) {
  j["id"]          = p.uuid_id_;
  j["revision"]      = p.revision_;
  // j["position"]      = p.position_;
  j["original_name"] = p.original_name_;
  j["extension"]     = p.extension_;
  j["width"]         = p.width_;
  j["height"]        = p.height_;
  j["duration"]      = p.duration_;
  j["status"]        = p.status_;
  j["annotations"]   = p.annotations_;
  j["previews"]      = p.previews_;
  j["created_at"]    = p.created_at_;
  j["task_id"]       = p.task_id_;
}
}  // namespace doodle