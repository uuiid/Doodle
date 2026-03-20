//
// Created by TD on 25-2-17.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <string>

namespace doodle::socket_io {
class sid_ctx;
void broadcast(
    const std::string& in_event, const nlohmann::json& in_data, const std::string& in_namespace = {"/events"},
    const std::shared_ptr<sid_ctx>& in_ctx = nullptr
);

template <typename To_Jsonable>
concept Jsonable = requires(const To_Jsonable& t) {
  { nlohmann::json{} = t };
};
template <Jsonable To_Jsonable>
void broadcast(
    const std::string& in_event, const To_Jsonable& in_data, const std::string& in_namespace = {"/events"},
    const std::shared_ptr<sid_ctx>& in_ctx = nullptr
) {
  broadcast(in_event, nlohmann::json{} = in_data, in_namespace, in_ctx);
}

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
  uuid asset_id;
  uuid asset_type;
  uuid project_id;
  // to json
  friend void to_json(nlohmann::json& j, const asset_new_broadcast_t& p) {
    j["asset_id"]   = p.asset_id;
    j["asset_type"] = p.asset_type;
    j["project_id"] = p.project_id;
  }
};

}  // namespace doodle::socket_io