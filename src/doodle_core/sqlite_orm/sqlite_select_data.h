//
// Created by TD on 25-3-25.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/attachment_file.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/preview_file.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>

#include <sqlite_orm/sqlite_orm.h>
namespace doodle {

struct DOODLE_CORE_API entity_task_t : entity {
  struct task_impl : task {
    bool is_subscribed_;
  };

  std::vector<task_impl> tasks_;
  std::shared_ptr<asset_type> asset_type_;
  // to json
  friend void to_json(nlohmann::json& j, const entity_task_t& p) {
    j["id"]                  = p.uuid_id_;
    j["name"]                = p.name_;
    j["preview_file_id"]     = p.preview_file_id_;
    j["description"]         = p.description_;
    j["asset_type_name"]     = p.asset_type_ ? p.asset_type_->name_ : std::string{};
    j["asset_type_id"]       = p.asset_type_ ? p.asset_type_->uuid_id_ : uuid{};
    j["canceled"]            = p.canceled_;
    j["ready_for"]           = p.ready_for_;
    j["episode_id"]          = p.source_id_;
    j["casting_episode_ids"] = nlohmann::json::array();
    j["is_casting_standby"]  = p.is_casting_standby_;
    j["is_shared"]           = p.is_shared_;
    j["data"]                = p.data_;
    j["tasks"]               = nlohmann::json::array();
    auto& l_task_json        = j["tasks"];
    for (const auto& task : p.tasks_) {
      nlohmann::json l_j_task{};
      l_j_task["id"]                   = task.uuid_id_;
      l_j_task["due_date"]             = task.due_date_;
      l_j_task["done_date"]            = task.done_date_;
      l_j_task["duration"]             = task.duration_;
      l_j_task["entity_id"]            = task.entity_id_;
      l_j_task["estimation"]           = task.estimation_;
      l_j_task["end_date"]             = task.end_date_;
      l_j_task["is_subscribed"]        = task.is_subscribed_;
      l_j_task["last_comment_date"]    = task.last_comment_date_;
      l_j_task["last_preview_file_id"] = task.last_preview_file_id_;
      l_j_task["priority"]             = task.priority_;
      l_j_task["real_start_date"]      = task.real_start_date_;
      l_j_task["retake_count"]         = task.retake_count_;
      l_j_task["start_date"]           = task.start_date_;
      l_j_task["difficulty"]           = task.difficulty_;
      l_j_task["task_status_id"]       = task.task_status_id_;
      l_j_task["task_type_id"]         = task.task_type_id_;
      l_j_task["assignees"]            = task.assignees_;
      l_j_task["path"]                 = "";
      l_task_json.emplace_back(std::move(l_j_task));
    }
  }
};

struct todo_t {
  // std::shared_ptr<project> project_;
  // std::shared_ptr<entity> entity_;
  decltype(task::uuid_id_) uuid_id_;
  decltype(task::name_) name_;
  decltype(task::description_) description_;
  decltype(task::priority_) priority_;
  decltype(task::difficulty_) difficulty_;
  decltype(task::duration_) duration_;
  decltype(task::estimation_) estimation_;
  decltype(task::completion_rate_) completion_rate_;
  decltype(task::retake_count_) retake_count_;
  decltype(task::sort_order_) sort_order_;
  decltype(task::start_date_) start_date_;
  decltype(task::due_date_) due_date_;
  decltype(task::real_start_date_) real_start_date_;
  decltype(task::end_date_) end_date_;
  decltype(task::done_date_) done_date_;
  decltype(task::last_comment_date_) last_comment_date_;
  decltype(task::nb_assets_ready_) nb_assets_ready_;
  decltype(task::data_) data_;
  decltype(task::shotgun_id_) shotgun_id_;
  decltype(task::last_preview_file_id_) last_preview_file_id_;
  decltype(task::project_id_) project_id_;
  decltype(task::task_type_id_) task_type_id_;
  decltype(task::task_status_id_) task_status_id_;
  decltype(task::assigner_id_) assigner_id_;
  decltype(task::created_at_) created_at_;
  decltype(task::updated_at_) updated_at_;

  decltype(project::name_) project_name_;
  decltype(project::has_avatar_) project_has_avatar_;

  decltype(entity::uuid_id_) entity_uuid_id_;
  decltype(entity::name_) entity_name_;
  decltype(entity::description_) entity_description_;
  decltype(entity::data_) entity_data_;
  decltype(entity::preview_file_id_) entity_preview_file_id_;

  decltype(asset_type::name_) asset_type_name_;
  decltype(entity::canceled_) entity_canceled_;
  decltype(entity::parent_id_) entity_parent_id_;
  decltype(entity::source_id_) entity_source_id_;
  decltype(task_type::name_) task_type_name_;
  decltype(task_type::for_entity_) task_type_for_entity_;
  decltype(task_type::color_) task_type_color_;

  decltype(task_status::name_) task_status_name_;
  decltype(task_status::color_) task_status_color_;
  decltype(task_status::short_name_) task_status_short_name_;

  std::vector<uuid> assignees_;

  struct comment_t {
    std::string text_;
    chrono::system_zoned_time date_;
    uuid person_id_;
    // to json
    friend void to_json(nlohmann::json& j, const comment_t& p) {
      j["text"]      = p.text_;
      j["date"]      = p.date_;
      j["person_id"] = p.person_id_;
    }
  };

  std::optional<comment_t> last_comment_;
  // to json
  friend void to_json(nlohmann::json& j, const todo_t& p) {
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
};

struct project_and_status_t {
  decltype(project::uuid_id_) uuid_id_;
  decltype(project::name_) name_;
  decltype(project::code_) code_;
  decltype(project::description_) description_;
  decltype(project::shotgun_id_) shotgun_id_;
  decltype(project::file_tree_) file_tree_;
  decltype(project::data_) data_;
  decltype(project::has_avatar_) has_avatar_;
  decltype(project::fps_) fps_;
  decltype(project::ratio_) ratio_;
  decltype(project::resolution_) resolution_;
  decltype(project::production_type_) production_type_;
  decltype(project::production_style_) production_style_;
  decltype(project::start_date_) start_date_;
  decltype(project::end_date_) end_date_;
  decltype(project::man_days_) man_days_;
  decltype(project::nb_episodes_) nb_episodes_;
  decltype(project::episode_span_) episode_span_;
  decltype(project::max_retakes_) max_retakes_;
  decltype(project::is_clients_isolated_) is_clients_isolated_;
  decltype(project::is_preview_download_allowed_) is_preview_download_allowed_;
  decltype(project::is_set_preview_automated_) is_set_preview_automated_;
  decltype(project::homepage_) homepage_;
  decltype(project::is_publish_default_for_artists_) is_publish_default_for_artists_;
  decltype(project::hd_bitrate_compression_) hd_bitrate_compression_;
  decltype(project::ld_bitrate_compression_) ld_bitrate_compression_;
  decltype(project::project_status_id_) project_status_id_;

  decltype(project::default_preview_background_file_id_) default_preview_background_file_id_;
  decltype(project_status::name_) project_status_name_;

  // to json
  friend void to_json(nlohmann::json& j, const project_and_status_t& p) {
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
};

struct get_comments_t {
  decltype(comment::uuid_id_) uuid_id_;
  decltype(comment::shotgun_id_) shotgun_id_;
  decltype(comment::object_id_) object_id_;
  decltype(comment::object_type_) object_type_;
  decltype(comment::text_) text_;
  decltype(comment::data_) data_;
  decltype(comment::replies_) replies_;
  decltype(comment::checklist_) checklist_;
  decltype(comment::pinned_) pinned_;
  decltype(comment::links) links_;
  decltype(comment::created_at_) created_at_;
  decltype(comment::updated_at_) updated_at_;
  decltype(comment::task_status_id_) task_status_id_;
  decltype(comment::person_id_) person_id_;
  decltype(comment::editor_id_) editor_id_;  // 编辑人
  decltype(comment::preview_file_id_) preview_file_id_;

  struct person_t {
    decltype(person::uuid_id_) uuid_id_;
    decltype(person::first_name_) first_name_;
    decltype(person::last_name_) last_name_;
    decltype(person::has_avatar_) has_avatar_;

    // to json
    friend void to_json(nlohmann::json& j, const person_t& p) {
      j["id"]         = p.uuid_id_;
      j["first_name"] = p.first_name_;
      j["last_name"]  = p.last_name_;
      j["has_avatar"] = p.has_avatar_;
    }
  };
  struct task_status_t {
    decltype(task_status::uuid_id_) uuid_id_;
    decltype(task_status::name_) name_;
    decltype(task_status::color_) color_;
    decltype(task_status::short_name_) short_name_;
    // to json
    friend void to_json(nlohmann::json& j, const task_status_t& p) {
      j["id"]         = p.uuid_id_;
      j["name"]       = p.name_;
      j["color"]      = p.color_;
      j["short_name"] = p.short_name_;
    }
  };
  struct previews_t {
    decltype(preview_file::uuid_id_) uuid_id_;
    decltype(preview_file::task_id_) task_id_;
    decltype(preview_file::revision_) revision_;
    decltype(preview_file::extension_) extension_;
    decltype(preview_file::width_) width_;
    decltype(preview_file::height_) height_;
    decltype(preview_file::duration_) duration_;
    decltype(preview_file::status_) status_;
    decltype(preview_file::validation_status_) validation_status_;
    decltype(preview_file::original_name_) original_name_;
    decltype(preview_file::position_) position_;
    decltype(preview_file::annotations_) annotations_;
    // to json
    friend void to_json(nlohmann::json& j, const previews_t& p) {
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
  };
  struct attachment_files_t {
    decltype(attachment_file::uuid_id_) uuid_id_;
    decltype(attachment_file::name_) name_;
    decltype(attachment_file::extension_) extension_;
    decltype(attachment_file::size_) size_;
    // to json
    friend void to_json(nlohmann::json& j, const attachment_files_t& p) {
      j["id"]        = p.uuid_id_;
      j["name"]      = p.name_;
      j["extension"] = p.extension_;
      j["size"]      = p.size_;
    }
  };

  std::vector<uuid> acknowledgements_;
  std::vector<uuid> mentions_;
  std::vector<uuid> department_mentions_;

  person_t persons_;
  std::optional<person_t> editors_;
  task_status_t task_statuses_;
  std::vector<previews_t> previews_;
  std::vector<attachment_files_t> attachment_files_;

  // to json
  friend void to_json(nlohmann::json& j, const get_comments_t& p) {
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
};

}  // namespace doodle
