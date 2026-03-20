//
// Created by TD on 25-2-17.
//

#pragma once

#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/comment.h"
#include "doodle_core/metadata/computer.h"
#include "doodle_core/metadata/preview_file.h"
#include "doodle_core/metadata/server_task_info.h"
#include "doodle_core/metadata/task_status.h"

#include <doodle_lib/doodle_lib_fwd.h>

#include <string>

namespace doodle::socket_io {
class sid_ctx;
void broadcast(
    const std::string& in_event, const nlohmann::json& in_data, const std::string& in_namespace = {"/events"},
    const std::shared_ptr<sid_ctx>& in_ctx = nullptr
);

/*
    结构体约束
    1. 结构体成员必须是可序列化 json 的类型(使用 nlohmann::json{} = value 的方式序列化)
    2. 结构体必须有编译器常量属性 event_name_ , 用于指定广播事件名称
    3. 结构体必须有编译器常量属性 namespace_，用于指定广播命名空间 (默认为 "/events")
*/
template <typename Struct>
concept BroadcastStruct = requires(const Struct& s) {
  { Struct::event_name_ } -> std::convertible_to<std::string_view>;
  { Struct::namespace_ } -> std::convertible_to<std::string_view>;
  { nlohmann::json{} = s } -> std::same_as<nlohmann::json&>;
};

template <BroadcastStruct Struct>
void broadcast(const Struct& in_struct, const std::shared_ptr<sid_ctx>& in_ctx = nullptr) {
  broadcast(std::string{Struct::event_name_}, nlohmann::json{} = in_struct, std::string{Struct::namespace_}, in_ctx);
}

struct asset_new_broadcast_t {
  static constexpr std::string_view event_name_ = "asset:new";
  static constexpr std::string_view namespace_  = "/events";
  uuid asset_id_;
  uuid asset_type_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const asset_new_broadcast_t& p) {
    j["asset_id"]   = p.asset_id_;
    j["asset_type"] = p.asset_type_;
    j["project_id"] = p.project_id_;
  }
};
struct entity_link_update_broadcast_t {
  static constexpr std::string_view event_name_ = "entity-link:update";
  static constexpr std::string_view namespace_  = "/events";
  uuid entity_link_id_;
  std::int32_t nb_occurences_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const entity_link_update_broadcast_t& p) {
    j["entity_link_id"] = p.entity_link_id_;
    j["nb_occurences"]  = p.nb_occurences_;
    j["project_id"]     = p.project_id_;
  }
};

struct asset_update_broadcast_t {
  static constexpr std::string_view event_name_ = "asset:update";
  static constexpr std::string_view namespace_  = "/events";
  uuid asset_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const asset_update_broadcast_t& p) {
    j["asset_id"]   = p.asset_id_;
    j["project_id"] = p.project_id_;
  }
};

struct entity_link_new_broadcast_t {
  static constexpr std::string_view event_name_ = "entity-link:new";
  static constexpr std::string_view namespace_  = "/events";
  uuid entity_link_id_;
  uuid entity_in_id_;
  uuid entity_out_id_;
  std::int32_t nb_occurences_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const entity_link_new_broadcast_t& p) {
    j["entity_link_id"] = p.entity_link_id_;
    j["entity_in_id"]   = p.entity_in_id_;
    j["entity_out_id"]  = p.entity_out_id_;
    j["nb_occurences"]  = p.nb_occurences_;
    j["project_id"]     = p.project_id_;
  }
};
struct shot_casting_update_broadcast_t {
  static constexpr std::string_view event_name_ = "shot-casting:update";
  static constexpr std::string_view namespace_  = "/events";
  uuid shot_id_;
  uuid project_id_;
  std::int32_t nb_entities_out_;
  // to json
  friend void to_json(nlohmann::json& j, const shot_casting_update_broadcast_t& p) {
    j["shot_id"]         = p.shot_id_;
    j["project_id"]      = p.project_id_;
    j["nb_entities_out"] = p.nb_entities_out_;
  }
};
struct task_update_broadcast_t {
  static constexpr std::string_view event_name_ = "task:update";
  static constexpr std::string_view namespace_  = "/events";
  uuid task_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const task_update_broadcast_t& p) {
    j["task_id"]    = p.task_id_;
    j["project_id"] = p.project_id_;
  }
};

struct task_status_change_broadcast_t {
  static constexpr std::string_view event_name_ = "task:status-changed";
  static constexpr std::string_view namespace_  = "/events";
  uuid task_id_;
  uuid new_task_status_id_;
  uuid previous_task_status_id_;
  uuid person_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const task_status_change_broadcast_t& p) {
    j["task_id"]                 = p.task_id_;
    j["new_task_status_id"]      = p.new_task_status_id_;
    j["previous_task_status_id"] = p.previous_task_status_id_;
    j["person_id"]               = p.person_id_;
    j["project_id"]              = p.project_id_;
  }
};

struct comment_new_broadcast_t {
  static constexpr std::string_view event_name_ = "comment:new";
  static constexpr std::string_view namespace_  = "/events";
  uuid comment_id_;
  uuid task_id_;
  uuid task_status_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const comment_new_broadcast_t& p) {
    j["comment_id"]     = p.comment_id_;
    j["task_id"]        = p.task_id_;
    j["task_status_id"] = p.task_status_id_;
    j["project_id"]     = p.project_id_;
  }
};
struct comment_acknowledge_broadcast_t {
  static constexpr std::string_view event_name_ = "comment:acknowledge";
  static constexpr std::string_view namespace_  = "/events";
  uuid comment_id_;
  uuid person_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const comment_acknowledge_broadcast_t& p) {
    j["comment_id"] = p.comment_id_;
    j["person_id"]  = p.person_id_;
    j["project_id"] = p.project_id_;
  }
};
struct comment_unacknowledge_broadcast_t {
  static constexpr std::string_view event_name_ = "comment:unacknowledge";
  static constexpr std::string_view namespace_  = "/events";
  uuid comment_id_;
  uuid person_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const comment_unacknowledge_broadcast_t& p) {
    j["comment_id"] = p.comment_id_;
    j["person_id"]  = p.person_id_;
    j["project_id"] = p.project_id_;
  }
};
struct computer_delete_broadcast_t {
  static constexpr std::string_view event_name_ = "computer:delete";
  static constexpr std::string_view namespace_  = "/events";
  uuid computer_id_;
  // to json
  friend void to_json(nlohmann::json& j, const computer_delete_broadcast_t& p) { j["computer_id"] = p.computer_id_; }
};
struct computer_update_broadcast_t {
  static constexpr std::string_view event_name_ = "computer:update";
  static constexpr std::string_view namespace_  = "/events";
  uuid computer_id_;
  // to json
  friend void to_json(nlohmann::json& j, const computer_update_broadcast_t& p) { j["computer_id"] = p.computer_id_; }
};

struct server_task_info_new_broadcast_t {
  static constexpr std::string_view event_name_ = "server-task-info:new";
  static constexpr std::string_view namespace_  = "/events";
  uuid server_task_info_id_;
  // to json
  friend void to_json(nlohmann::json& j, const server_task_info_new_broadcast_t& p) {
    j["server_task_info_id"] = p.server_task_info_id_;
  }
};

struct server_task_info_update_broadcast_t {
  static constexpr std::string_view event_name_ = "server-task-info:update";
  static constexpr std::string_view namespace_  = "/events";
  uuid server_task_info_id_;
  // to json
  friend void to_json(nlohmann::json& j, const server_task_info_update_broadcast_t& p) {
    j["server_task_info_id"] = p.server_task_info_id_;
  }
};

struct server_task_info_delete_broadcast_t {
  static constexpr std::string_view event_name_ = "server-task-info:delete";
  static constexpr std::string_view namespace_  = "/events";
  uuid server_task_info_id_;
  // to json
  friend void to_json(nlohmann::json& j, const server_task_info_delete_broadcast_t& p) {
    j["server_task_info_id"] = p.server_task_info_id_;
  }
};

struct organisation_set_thumbnail_broadcast_t {
  static constexpr std::string_view event_name_ = "organisation:set-thumbnail";
  static constexpr std::string_view namespace_  = "/events";
  uuid organisation_id_;
  // to json
  friend void to_json(nlohmann::json& j, const organisation_set_thumbnail_broadcast_t& p) {
    j["organisation_id"] = p.organisation_id_;
  }
};

struct preview_file_annotation_update_broadcast_t {
  static constexpr std::string_view event_name_ = "preview-file:annotation-update";
  static constexpr std::string_view namespace_  = "/events";
  uuid preview_file_id_;
  uuid person_id_;
  chrono::system_zoned_time updated_at_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const preview_file_annotation_update_broadcast_t& p) {
    j["preview_file_id"] = p.preview_file_id_;
    j["project_id"]      = p.project_id_;
  }
};
struct preview_file_new_broadcast_t {
  static constexpr std::string_view event_name_ = "preview-file:new";
  static constexpr std::string_view namespace_  = "/events";
  uuid preview_file_id_;
  uuid comment_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const preview_file_new_broadcast_t& p) {
    j["preview_file_id"] = p.preview_file_id_;
    j["comment_id"]      = p.comment_id_;
    j["project_id"]      = p.project_id_;
  }
};
struct comment_update_broadcast_t {
  static constexpr std::string_view event_name_ = "comment:update";
  static constexpr std::string_view namespace_  = "/events";
  uuid comment_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const comment_update_broadcast_t& p) {
    j["comment_id"] = p.comment_id_;
    j["project_id"] = p.project_id_;
  }
};
struct task_new_broadcast_t {
  static constexpr std::string_view event_name_ = "task:new";
  static constexpr std::string_view namespace_  = "/events";
  uuid task_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const task_new_broadcast_t& p) {
    j["task_id"]    = p.task_id_;
    j["project_id"] = p.project_id_;
  }
};
struct project_update_broadcast_t {
  static constexpr std::string_view event_name_ = "project:update";
  static constexpr std::string_view namespace_  = "/events";
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const project_update_broadcast_t& p) { j["project_id"] = p.project_id_; }
};
struct sequence_new_broadcast_t {
  static constexpr std::string_view event_name_ = "sequence:new";
  static constexpr std::string_view namespace_  = "/events";
  uuid sequence_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const sequence_new_broadcast_t& p) {
    j["sequence_id"] = p.sequence_id_;
    j["project_id"]  = p.project_id_;
  }
};
struct sequence_update_broadcast_t {
  static constexpr std::string_view event_name_ = "sequence:update";
  static constexpr std::string_view namespace_  = "/events";
  uuid sequence_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const sequence_update_broadcast_t& p) {
    j["sequence_id"] = p.sequence_id_;
    j["project_id"]  = p.project_id_;
  }
};
struct sequence_delete_broadcast_t {
  static constexpr std::string_view event_name_ = "sequence:delete";
  static constexpr std::string_view namespace_  = "/events";
  uuid sequence_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const sequence_delete_broadcast_t& p) {
    j["sequence_id"] = p.sequence_id_;
    j["project_id"]  = p.project_id_;
  }
};
struct shot_new_broadcast_t {
  static constexpr std::string_view event_name_ = "shot:new";
  static constexpr std::string_view namespace_  = "/events";
  uuid shot_id_;
  uuid episode_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const shot_new_broadcast_t& p) {
    j["shot_id"]    = p.shot_id_;
    j["episode_id"] = p.episode_id_;
    j["project_id"] = p.project_id_;
  }
};
struct shot_update_broadcast_t {
  static constexpr std::string_view event_name_ = "shot:update";
  static constexpr std::string_view namespace_  = "/events";
  uuid shot_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const shot_update_broadcast_t& p) {
    j["shot_id"]    = p.shot_id_;
    j["project_id"] = p.project_id_;
  }
};
struct shot_delete_broadcast_t {
  static constexpr std::string_view event_name_ = "shot:delete";
  static constexpr std::string_view namespace_  = "/events";
  uuid shot_id_;
  uuid project_id_;
  // to json
  friend void to_json(nlohmann::json& j, const shot_delete_broadcast_t& p) {
    j["shot_id"]    = p.shot_id_;
    j["project_id"] = p.project_id_;
  }
};
struct tools_add_watermark_progress_broadcast_t {
  static constexpr std::string_view event_name_ = "tools:add-watermark:progress";
  static constexpr std::string_view namespace_  = "/events";
  uuid id_;
  double progress_;
  FSys::path image_path_;
  FSys::path output_path_;
  // to json
  friend void to_json(nlohmann::json& j, const tools_add_watermark_progress_broadcast_t& p) {
    j["id"]          = p.id_;
    j["progress"]    = p.progress_;
    j["image_path"]  = p.image_path_;
    j["output_path"] = p.output_path_;
  }
};

struct local_server_task_info_progress_broadcast_t {
  static constexpr std::string_view event_name_ = "doodle:task_info:progress";
  static constexpr std::string_view namespace_  = "/events";
  uuid id_;
  double progress_;
  // to json
  friend void to_json(nlohmann::json& j, const local_server_task_info_progress_broadcast_t& p) {
    j["id"]       = p.id_;
    j["progress"] = p.progress_;
  }
};
struct local_server_task_info_update_broadcast_t {
  static constexpr std::string_view event_name_ = "doodle:task_info:update";
  static constexpr std::string_view namespace_  = "/events";
  server_task_info main_info_;
  // to json
  friend void to_json(nlohmann::json& j, const local_server_task_info_update_broadcast_t& p) { j = p.main_info_; }
};

struct preview_file_progress_update_broadcast_t {
  static constexpr std::string_view event_name_ = "preview-file:progress-update";
  static constexpr std::string_view namespace_  = "/events";
  uuid preview_file_id_;
  double progress_;
  // to json
  friend void to_json(nlohmann::json& j, const preview_file_progress_update_broadcast_t& p) {
    j["preview_file_id"] = p.preview_file_id_;
    j["progress"]        = p.progress_;
  }
};

}  // namespace doodle::socket_io