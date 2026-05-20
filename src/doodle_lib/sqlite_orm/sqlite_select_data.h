//
// Created by TD on 25-3-25.
//

#pragma once
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/person.h"
#include <doodle_core/metadata/ai_studio.h>
#include <doodle_core/metadata/attachment_file.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/preview_file.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/seedance2/assets_entity.h>
#include <doodle_core/metadata/seedance2/assets_entity_item.h>
#include <doodle_core/metadata/seedance2/group.h>
#include <doodle_core/metadata/seedance2/task.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <optional>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace doodle {
struct working_file_and_link;
struct outsource_studio_authorization;
struct notification;

enum class notification_type;
struct computer;
struct playlist;
struct todo_t {
  // std::shared_ptr<project> project_;
  // std::shared_ptr<entity> entity_;

  explicit todo_t(
      const task& in_task, const std::string& in_prj_name, bool in_has_avatar, const entity& in_entity,
      const decltype(asset_type::name_)& in_asset_type, const decltype(task_type::name_)& in_task_type_name,
      const decltype(task_type::for_entity_)& in_task_type_for_entity,
      const decltype(task_type::color_)& in_task_type_color, const decltype(task_status::name_)& in_task_status_name,
      const decltype(task_status::color_)& in_task_status_color,
      const decltype(task_status::short_name_)& in_task_status_short_name,
      const entity_asset_extend& in_entity_asset_extend
  )
      : uuid_id_(in_task.uuid_id_),
        name_(in_task.name_),
        description_(in_task.description_),
        priority_(in_task.priority_),
        difficulty_(in_task.difficulty_),
        duration_(in_task.duration_),
        estimation_(in_task.estimation_),
        completion_rate_(in_task.completion_rate_),
        retake_count_(in_task.retake_count_),
        sort_order_(in_task.sort_order_),
        start_date_(in_task.start_date_),
        due_date_(in_task.due_date_),
        real_start_date_(in_task.real_start_date_),
        end_date_(in_task.end_date_),
        done_date_(in_task.done_date_),
        last_comment_date_(in_task.last_comment_date_),
        nb_assets_ready_(in_task.nb_assets_ready_),
        data_(in_task.data_),
        shotgun_id_(in_task.shotgun_id_),
        last_preview_file_id_(in_task.last_preview_file_id_),
        project_id_(in_task.project_id_),
        task_type_id_(in_task.task_type_id_),
        task_status_id_(in_task.task_status_id_),
        assigner_id_(in_task.assigner_id_),
        created_at_(in_task.created_at_),
        updated_at_(in_task.updated_at_),
        project_name_(in_prj_name),
        project_has_avatar_(in_has_avatar),
        entity_uuid_id_(in_entity.uuid_id_),
        entity_name_(in_entity.name_),
        entity_description_(in_entity.description_),
        entity_preview_file_id_(in_entity.preview_file_id_),
        asset_type_name_(in_asset_type),
        entity_canceled_(in_entity.canceled_),
        entity_parent_id_(in_entity.parent_id_),
        entity_source_id_(in_entity.source_id_),
        task_type_name_(in_task_type_name),
        task_type_for_entity_(in_task_type_for_entity),
        task_type_color_(in_task_type_color),
        task_status_name_(in_task_status_name),
        task_status_color_(in_task_status_color),
        task_status_short_name_(in_task_status_short_name),
        entity_asset_extend_id_(in_entity_asset_extend.uuid_id_),
        ji_shu_lie_(in_entity_asset_extend.ji_shu_lie_),
        deng_ji_(in_entity_asset_extend.deng_ji_),
        gui_dang_(in_entity_asset_extend.gui_dang_),
        bian_hao_(in_entity_asset_extend.bian_hao_),
        pin_yin_ming_cheng_(in_entity_asset_extend.pin_yin_ming_cheng_),
        ban_ben_(in_entity_asset_extend.ban_ben_),
        ji_du_(in_entity_asset_extend.ji_du_),
        assignees_(),
        last_comment_(std::nullopt)

  {}

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
  decltype(entity_asset_extend::uuid_id_) entity_asset_extend_id_;
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
struct assets_entity_and_item : public seedance2::assets_entity {
  explicit assets_entity_and_item(const seedance2::assets_entity& in_entity) : seedance2::assets_entity(in_entity) {}

  std::vector<seedance2::assets_entity_item> items_;
  // to json
  friend void to_json(nlohmann::json& j, const assets_entity_and_item& p) {
    to_json(j, static_cast<const seedance2::assets_entity>(p));
    j["items"] = p.items_;
  }
};

namespace sqlite_select {
namespace sd2 = doodle::seedance2;

std::vector<ai_studio_and_link_t> ai_studio_and_link_t_get_all();
// 从实体查询绑定人员
std::string get_rig_person_last_name_for_entity(const uuid& in_entity_id);
// 从人员中获取 ai工作室 uuid
uuid get_ai_studio_uuid_for_person(const uuid& in_person_id);
std::optional<assignees_table> get_task_assignees_for_task_and_person(uuid in_task_id, uuid in_person_id);
std::vector<std::int64_t> get_task_assignees_ids_for_task(uuid in_task_id);
// 使用 ai 工作室筛选 seedance2::task
std::vector<sd2::task> get_sd2_tasks_for_ai_studio(const uuid& in_ai_studio_id);
// 使用 人员id 筛选 seedance2::task
std::vector<sd2::task> get_sd2_tasks_for_person(const uuid& in_person_id);
// 使用 ai 工作室筛选 seedance2::assets_group
std::vector<sd2::assets_group> get_sd2_assets_group_for_ai_studio(const uuid& in_ai_studio_id);
// 获取 seedance2::assets_group 组下发资产数量
std::size_t get_sd2_assets_count_for_assets_group(const uuid& in_assets_group_id);
std::vector<assets_entity_and_item> get_assets_entity_and_item_all_for_person_and_ai_studio(
    const uuid& in_group_id, const uuid& in_ai_studio_id
);
// 使用名称搜索 seedance2::assets_entity
std::vector<sd2::assets_entity> search_sd2_assets_entity_for_ai_studio(
    const uuid& in_ai_studio_id, const std::string& keyword
);
// 实体是否有解算资产
bool entity_has_simulation_asset(const uuid& in_entity_id);
// 获取资产和资产扩展数据 使用项目和镜头, 集数过滤
std::vector<std::tuple<entity, entity_asset_extend>> get_working_files_for_entity(
    const uuid& in_project_id, const uuid& in_shot_id, const uuid& in_sequence_id
);
// 获取资产和资产扩展数据 使用项目和镜头, 集数过滤
std::vector<std::tuple<entity, entity_asset_extend>> get_working_files_for_entity(const uuid& in_entity_id);
std::vector<std::tuple<entity, entity_asset_extend>> get_working_files_for_entity(
    const std::vector<uuid>& in_entity_ids
);
// 从项目,集数人员, 镜头获取序列投射数据
std::vector<std::tuple<entity_link, std::string, uuid, uuid, std::string>>
get_sequence_casting_for_project_and_person_and_sequence(
    const uuid& in_project_id, const person& in_person, const uuid& in_sequence_id, const std::vector<uuid>& in_shot_ids
);
// 从项目, 资产类型 获取序列投射数据
std::vector<std::tuple<entity_link, std::string, uuid, uuid, std::string>>
get_sequence_casting_for_project_and_asset_type(const uuid& in_project_id, const uuid& in_asset_type_id);
std::vector<std::tuple<entity_link, std::string, std::string, uuid, uuid, uuid, uuid>> get_sequence_casting_for_entity(
    const uuid& in_entity_id
);
// 从镜头获取关联的 资产链接
std::vector<entity_link> get_entity_link_by_entity_id(const uuid& in_entity_id);
std::vector<entity_link> get_entity_link_by_entity_id(const std::vector<uuid>& in_entity_id);

std::vector<std::tuple<entity, outsource_studio_authorization, entity_asset_extend, entity_shot_extend>>
get_entity_and_outsource_studio_authorization_by_project_id(const uuid& in_project_id);

std::vector<std::tuple<entity, entity_asset_extend>> get_entity_and_entity_asset_extend_by_shot_id(
    const uuid& in_shot_id
);
std::vector<preview_file> get_preview_files_by_entity_id(const uuid& in_entity_id);
std::optional<entity_asset_extend> get_entity_asset_extend_by_entity_id(const uuid& in_entity_id);
std::optional<entity_shot_extend> get_entity_shot_extend_by_entity_id(const uuid& in_entity_id);
std::optional<computer> get_entity_computer_by_hardware_id(const uuid& in_hardware_id);
std::vector<uuid> get_comment_object_ids_by_comment_id(const uuid& in_comment_id);
std::vector<uuid> get_task_project_ids_by_task_id(const uuid& in_task_id);
std::vector<std::int32_t> get_comment_acknowledgement_ids_by_comment_id_and_person_id(
    const uuid& in_comment_id, const uuid& in_person_id
);
std::vector<attachment_file> get_attachment_files_by_comment_id(const uuid& in_comment_id);

struct get_playlist_by_task_type_and_project {
  enum order_by_enum {
    create_at,
    name,
    update_at,
  };
  order_by_enum order_by_;
  uuid project_id_;
  uuid task_type_id_;
  std::int32_t page_;

  std::vector<playlist> operator()() const;
};

std::vector<std::tuple<preview_file, uuid, uuid>> get_preview_files_and_task_type_id_and_task_entity_id_in_entity_ids(
    const std::vector<uuid>& in_entity_ids
);
std::size_t count_playlist_shots_by_playlist_shot_id(const uuid& in_playlist_shot_id);
std::vector<std::tuple<preview_file, uuid, std::string>> get_preview_files_and_entity_id_and_entity_name_by_sequence_id(
    const uuid& in_sequence_id
);

std::vector<std::tuple<notification, entity, comment, uuid, std::string, uuid, uuid>>
get_notifications_and_entity_and_comment_and_project_id_and_project_name_and_task_id_and_task_name_by_person_id(
    const uuid& in_person_id, const std::optional<chrono::system_zoned_time>& in_after,
    const std::optional<chrono::system_zoned_time>& in_before, const uuid& in_task_type_id,
    const uuid& in_task_status_id, const std::optional<notification_type>& in_notification_type,
    const std::optional<bool>& in_read
);
std::vector<uuid> get_comment_mentions_person_ids_by_comment_id(const uuid& in_comment_id);
std::vector<uuid> get_comment_department_mentions_department_ids_by_comment_id(const uuid& in_comment_id);

std::optional<preview_file> get_preview_files_by_entity_id_and_simulation_task_type_and_lighting_animation(
    const uuid& in_entity_id
);
std::vector<attachment_file> get_attachment_files_by_comment_id_and_task_id(const uuid& in_task_id);
std::vector<std::tuple<uuid, std::string>> get_project_ids_and_names();
std::vector<std::tuple<
    uuid,                 // task::uuid_id_
    std::string,          // task::name_
    uuid,                 // task::last_preview_file_id_
    uuid,                 // entity::uuid_id_
    std::string,          // entity::name_
    uuid,                 // task_type::uuid_id_
    entity_asset_extend,  // entity_asset_extend
    uuid,                 // project::uuid_id_
    std::string           // project::name_
    >>
get_tasks_and_entities_and_entity_asset_extend_and_project_by_task_ids(const std::vector<uuid>& in_task_ids);
std::vector<std::int32_t> get_work_xlsx_task_info_helper_database_t_id_by_person_id_and_year_month(
    const uuid& in_person_id, const chrono::local_days& in_year_month
);
std::vector<std::tuple<project, std::string>> get_projects_and_status_name_by_project_name(
    const std::string& in_project_name
);
std::optional<std::int64_t> get_project_person_id_by_project_id_and_person_id(
    const uuid& in_project_id, const uuid& in_person_id
);
std::optional<std::int64_t> get_project_status_automation_id_by_project_id_and_status_id(
    const uuid& in_project_id, const uuid& in_status_id
);

std::vector<entity> get_entities_by_person_id_and_is_admin_and_is_shared(
    const uuid& in_person_id, bool in_is_admin, bool in_is_shared
);
std::vector<entity_fts> search_entities_fts_by_keyword(
    const std::string& in_keyword, const uuid& in_project_id, const std::int64_t in_limit, const std::int64_t in_offset
);

struct make_with_tasks_sql_result_t {
  person& person_;
  uuid id_;
  uuid project_id_;
  std::int32_t offset_{0};
  std::int32_t limit_{300};
  std::vector<uuid> entity_type_id_;
  std::vector<std::int32_t> ji_du_filter_;
  bool ji_du_filter_is_null{false};
  std::vector<std::int32_t> ji_shu_lie_filter_;
  bool ji_shu_lie_filter_is_null{false};
  std::vector<uuid> task_status_id_filter_;
  std::vector<uuid> person_id_filter_;
  std::string search_key_;
  std::vector<std::int32_t> scenes_;
  bool scenes_is_null{false};
  std::vector<std::tuple<entity, task, entity_asset_extend, asset_type, uuid>> operator()() const;
};

struct actions_projects_casting_copy_select {
  std::vector<entity_link> source_casting_;
  std::vector<entity> source_shots_;
  std::vector<entity> target_shots_;
};
actions_projects_casting_copy_select get_actions_projects_casting_copy_select(
    const uuid& in_source_project_id, const uuid& in_target_project_id
);

struct actions_projects_sequences_casting_ue_assembly_harvest_select_t {
  std::vector<std::tuple<entity_asset_extend, uuid /*entity::entity_type_id_*/>> ass_all_{};
  std::vector<std::tuple<entity, entity_shot_extend>> shot_and_ext_{};
  std::vector<std::tuple<entity_link, entity_asset_extend, uuid /*entity::entity_type_id_*/>> ass_link{};

  static actions_projects_sequences_casting_ue_assembly_harvest_select_t get(
      const uuid& in_project_id, const uuid& in_sequence_id
  );
};

struct get_get_entities_and_tasks_select_t {
  std::vector<std::tuple<entity, task, uuid>> entity_and_task_and_person_id_;
  std::vector<std::tuple<uuid, std::int32_t>> sequence_and_cout_;

  static get_get_entities_and_tasks_select_t get(
      const person& in_person, const uuid& in_project_id, const uuid& in_entity_type_id, std::int32_t in_offset = 0,
      std::int32_t in_limit = 300
  );
};

std::vector<entity> get_entity_by_episode_id_and_project_id_and_name(
    const uuid& type_id_, const uuid& in_episode_id, const uuid& in_project_id, const std::string& in_name
);

bool task_exit_by_entity_id_and_task_type_id(const uuid& in_entity_id, const uuid& in_task_type_id);

struct sd2_select_task_t {
  sd2::task task_;

  struct entity_and_task_t {
    uuid entity_id_;
    uuid task_id_;
    std::string name_;
    std::string code_;
    std::string description_;
    bool canceled_;
    std::optional<std::int32_t> nb_frames_;
    std::int32_t nb_entities_out_;
    bool is_casting_standby_;

    bool is_shared_;
    entity_status status_;

    uuid project_id_;
    uuid entity_type_id_;
    uuid parent_id_;
    uuid source_id_;
    uuid preview_file_id_;
    uuid ready_for_;
    uuid created_by_;
    std::optional<std::int32_t> frame_in_;
    std::optional<std::int32_t> frame_out_;
    // to json
    friend void to_json(nlohmann::json& j, const entity_and_task_t& p) {
      j["entity_id"]          = p.entity_id_;
      j["task_id"]            = p.task_id_;
      j["name"]               = p.name_;
      j["code"]               = p.code_;
      j["description"]        = p.description_;
      j["canceled"]           = p.canceled_;
      j["nb_frames"]          = p.nb_frames_;
      j["nb_entities_out"]    = p.nb_entities_out_;
      j["is_casting_standby"] = p.is_casting_standby_;
      j["is_shared"]          = p.is_shared_;
      j["status"]             = p.status_;
      j["project_id"]         = p.project_id_;
      j["entity_type_id"]     = p.entity_type_id_;
      j["parent_id"]          = p.parent_id_;
      j["source_id"]          = p.source_id_;
      j["preview_file_id"]    = p.preview_file_id_;
      j["ready_for"]          = p.ready_for_;
      j["created_by"]         = p.created_by_;
      j["frame_in"]           = p.frame_in_;
      j["frame_out"]          = p.frame_out_;
    }
  };

  entity_and_task_t entity_and_task_;
  // to json
  friend void to_json(nlohmann::json& j, const sd2_select_task_t& p) {
    j["task"]            = p.task_;
    j["entity_and_task"] = p.entity_and_task_;
  }

  explicit sd2_select_task_t(
      const sd2::task& in_sd2_task, const entity& in_entity, const task& in_task,
      const entity_shot_extend& in_entity_shot_extend
  )
      : task_(in_sd2_task),
        entity_and_task_{
            .entity_id_          = in_entity.uuid_id_,
            .task_id_            = in_task.uuid_id_,
            .name_               = in_entity.name_,
            .code_               = in_entity.code_,
            .description_        = in_entity.description_,
            .canceled_           = in_entity.canceled_,
            .nb_frames_          = in_entity.nb_frames_,
            .nb_entities_out_    = in_entity.nb_entities_out_,
            .is_casting_standby_ = in_entity.is_casting_standby_,
            .is_shared_          = in_entity.is_shared_,
            .status_             = in_entity.status_,
            .project_id_         = in_entity.project_id_,
            .entity_type_id_     = in_entity.entity_type_id_,
            .parent_id_          = in_entity.parent_id_,
            .source_id_          = in_entity.source_id_,
            .preview_file_id_    = in_entity.preview_file_id_,
            .ready_for_          = in_entity.ready_for_,
            .created_by_         = in_entity.created_by_,
            .frame_in_           = in_entity_shot_extend.frame_in_,
            .frame_out_          = in_entity_shot_extend.frame_out_
        } {}
};
std::vector<sd2_select_task_t> get_tasks_and_entity_for_ai_studio(const uuid& in_ai_studio_id);
std::vector<sd2_select_task_t> get_tasks_and_entity_for_person(const uuid& in_person_id);
std::vector<sd2::task> get_task_for_shot_task_id(const uuid& in_task_id, const uuid& in_ai_studio_id);
}  // namespace sqlite_select
}  // namespace doodle
