//
// Created by teXiao on 2021/4/27.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include "boost/operators.hpp"

#include "entt/entity/fwd.hpp"
#include <entt/entt.hpp>
namespace doodle {

namespace details {
enum class assets_type_enum {
  scene,
  prop,
  character,
  rig,
  animation,
  vfx,
  cfx,
  other,
};
}
/**
 * @brief 资产类, 这个时候资产分类
 *
 * 使用path表示资产的分类, 其中每段路径代表一个标签, 标签越靠前权重越高
 *
 */
class DOODLE_CORE_API assets : boost::totally_ordered<assets> {
  /**
   * @brief 分解路径,转换为向量缓存
   *
   */

  entt::handle parent_{};
  std::set<entt::handle> child_{};

 public:
  std::string p_path;

  assets() = default;

  explicit assets(std::string in_name) : p_path(std::move(in_name)){};

  assets(const assets& in_other)            = default;
  assets(assets&& in_other)                 = default;
  assets& operator=(const assets& in_other) = default;
  assets& operator=(assets&& in_other)      = default;

  [[nodiscard]] std::string str() const;

  void set_path(const std::string& in_path);

  [[nodiscard]] const std::string& get_path() const;

  [[nodiscard]] inline entt::handle get_parent() const { return parent_; }
  [[nodiscard]] std::set<entt::handle> get_child() const;

  entt::handle get_root() const;

  void add_child(const entt::handle& in_child);
  void remove_child(const entt::handle& in_child);
  void set_parent(const entt::handle& in_parent);

  bool operator<(const assets& in_rhs) const;
  bool operator!=(const assets& in_rhs) const;

  static void merge_assets_tree(const registry_ptr& in_registry_ptr);

 private:
  /**
   * @brief 序列化函数
   *
   * @param j json
   * @param p 资产
   */
  friend void to_json(nlohmann::json& j, const assets& p) {
    j["id"]   = p.parent_.entity();
    j["path"] = p.p_path;
  }
  /**
   * @copydoc to_json(nlohmann::json& j, const assets& p)
   */
  friend void from_json(const nlohmann::json& j, assets& p) { j.at("path").get_to(p.p_path); }
};

namespace assets_helper {
struct database_t {
  std::int32_t id_{};
  uuid uuid_id_{};
  std::string label_{};
  /// 这个数据不在数据库中
  std::optional<uuid> uuid_parent_{};
  friend void to_json(nlohmann::json& j, const database_t& v) {
    j["id"]    = v.uuid_id_;
    j["label"] = v.label_;
    if (v.uuid_parent_) j["parent_id"] = *v.uuid_parent_;
  }

  friend void from_json(const nlohmann::json& j, database_t& v) {
    j.at("label").get_to(v.label_);
    if (j.contains("parent_id")) j.at("parent_id").get_to(v.uuid_parent_);
  }
};
}  // namespace assets_helper
}  // namespace doodle
namespace fmt {
/**
 * @brief 格式化资产类
 * 这个使用资产的路径属性格式化
 * @tparam 资产类
 */
template <>
struct formatter<::doodle::assets> : formatter<std::string_view> {
  /**
   * @brief 格式化函数
   *
   * @tparam FormatContext fmt上下文类
   * @param in_ 传入的资产类
   * @param ctx 上下文
   * @return decltype(ctx.out()) 基本上时 std::string
   */
  template <typename FormatContext>
  auto format(const ::doodle::assets& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<std::string_view>::format(in_.p_path, ctx);
  }
};
}  // namespace fmt
