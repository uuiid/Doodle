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
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>

#include <sqlite_orm/sqlite_orm.h>
namespace doodle {

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

  // 额外的资产数据
  decltype(entity_asset_extend::ji_shu_lie_) ji_shu_lie_;
  decltype(entity_asset_extend::deng_ji_) deng_ji_;
  decltype(entity_asset_extend::gui_dang_) gui_dang_;
  decltype(entity_asset_extend::bian_hao_) bian_hao_;
  decltype(entity_asset_extend::pin_yin_ming_cheng_) pin_yin_ming_cheng_;
  decltype(entity_asset_extend::ban_ben_) ban_ben_;
  decltype(entity_asset_extend::ji_du_) ji_du_;

  std::vector<uuid> assignees_;

  struct comment_t {
    std::string text_;
    chrono::system_zoned_time date_;
    uuid person_id_;
    // to json
    friend void to_json(nlohmann::json& j, const comment_t& p);
  };

  std::optional<comment_t> last_comment_;
  // to json
  friend void to_json(nlohmann::json& j, const todo_t& p);
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
  friend void to_json(nlohmann::json& j, const project_and_status_t& p);
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
    friend void to_json(nlohmann::json& j, const person_t& p);
  };
  struct task_status_t {
    decltype(task_status::uuid_id_) uuid_id_;
    decltype(task_status::name_) name_;
    decltype(task_status::color_) color_;
    decltype(task_status::short_name_) short_name_;
    // to json
    friend void to_json(nlohmann::json& j, const task_status_t& p);
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
    friend void to_json(nlohmann::json& j, const previews_t& p);
  };
  struct attachment_files_t {
    decltype(attachment_file::uuid_id_) uuid_id_;
    decltype(attachment_file::name_) name_;
    decltype(attachment_file::extension_) extension_;
    decltype(attachment_file::size_) size_;
    // to json
    friend void to_json(nlohmann::json& j, const attachment_files_t& p);
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
  friend void to_json(nlohmann::json& j, const get_comments_t& p);
};

struct assets_and_tasks_t {
  decltype(entity::uuid_id_) uuid_id_;
  decltype(entity::name_) name_;
  decltype(entity::preview_file_id_) preview_file_id_;
  decltype(entity::description_) description_;
  decltype(asset_type::name_) asset_type_name_;
  decltype(asset_type::uuid_id_) asset_type_id_;
  decltype(entity::canceled_) canceled_;
  decltype(entity::ready_for_) ready_for_;
  decltype(entity::source_id_) source_id_;
  decltype(entity::is_casting_standby_) is_casting_standby_;
  decltype(entity::is_shared_) is_shared_;
  std::vector<uuid> casting_episode_ids_;
  // 额外的资产数据
  decltype(entity_asset_extend::ji_shu_lie_) ji_shu_lie_;
  decltype(entity_asset_extend::deng_ji_) deng_ji_;
  decltype(entity_asset_extend::gui_dang_) gui_dang_;
  decltype(entity_asset_extend::bian_hao_) bian_hao_;
  decltype(entity_asset_extend::pin_yin_ming_cheng_) pin_yin_ming_cheng_;
  decltype(entity_asset_extend::ban_ben_) ban_ben_;
  decltype(entity_asset_extend::ji_du_) ji_du_;
  struct task_t {
    decltype(task::uuid_id_) uuid_id_;
    decltype(task::due_date_) due_date_;
    decltype(task::done_date_) done_date_;
    decltype(task::duration_) duration_;
    decltype(task::entity_id_) entity_id_;
    decltype(task::estimation_) estimation_;
    decltype(task::end_date_) end_date_;
    decltype(task::last_comment_date_) last_comment_date_;
    decltype(task::last_preview_file_id_) last_preview_file_id_;
    decltype(task::priority_) priority_;
    decltype(task::real_start_date_) real_start_date_;
    decltype(task::retake_count_) retake_count_;
    decltype(task::start_date_) start_date_;
    decltype(task::difficulty_) difficulty_;
    decltype(task::task_type_id_) task_type_id_;
    decltype(task::task_status_id_) task_status_id_;
    std::vector<decltype(task::assigner_id_)> assigner_ids_;
    bool is_subscribed_;

    // to json
    friend void to_json(nlohmann::json& j, const task_t& p);
  };
  std::vector<task_t> tasks_;

  // to json
  friend void to_json(nlohmann::json& j, const assets_and_tasks_t& p);
};

struct entities_and_tasks_t {
  decltype(entity::uuid_id_) uuid_id_;
  decltype(entity::name_) name_;
  decltype(entity::status_) status_;
  decltype(entity::uuid_id_) episode_id_;
  decltype(entity::description_) description_;
  decltype(entity::preview_file_id_) preview_file_id_;
  decltype(entity::canceled_) canceled_;

  std::int32_t frame_in_;
  std::int32_t frame_out_;
  std::int32_t fps_;

  struct task_t {
    decltype(task::uuid_id_) uuid_id_;
    decltype(task::estimation_) estimation_;
    decltype(entity::uuid_id_) entity_id_;
    decltype(task::end_date_) end_date_;
    decltype(task::due_date_) due_date_;
    decltype(task::done_date_) done_date_;
    decltype(task::duration_) duration_;
    decltype(task::last_comment_date_) last_comment_date_;
    decltype(task::last_preview_file_id_) last_preview_file_id_;
    decltype(task::priority_) priority_;
    decltype(task::real_start_date_) real_start_date_;
    decltype(task::retake_count_) retake_count_;
    decltype(task::start_date_) start_date_;
    decltype(task::difficulty_) difficulty_;
    decltype(task::task_status_id_) task_status_id_;
    decltype(task::task_type_id_) task_type_id_;
    std::vector<uuid> assigners_;
    bool is_subscribed_;
    friend void to_json(nlohmann::json& j, const task_t& p);
  };
  std::vector<task_t> tasks_;
  // to json
  friend void to_json(nlohmann::json& j, const entities_and_tasks_t& p);
};

struct preview_files_for_entity_t {
  decltype(preview_file::uuid_id_) uuid_id_;
  decltype(preview_file::revision_) revision_;
  decltype(preview_file::position_) position_;
  decltype(preview_file::original_name_) original_name_;
  decltype(preview_file::extension_) extension_;
  decltype(preview_file::width_) width_;
  decltype(preview_file::height_) height_;
  decltype(preview_file::duration_) duration_;
  decltype(preview_file::status_) status_;
  decltype(preview_file::source_) source_;
  decltype(preview_file::annotations_) annotations_;
  decltype(preview_file::created_at_) created_at_;
  decltype(preview_file::task_id_) task_id_;
  decltype(task_type::uuid_id_) task_type_id_;

  std::vector<preview_files_for_entity_t> previews_;

  // to json
  friend void to_json(nlohmann::json& j, const preview_files_for_entity_t& p);
};

}  // namespace doodle
