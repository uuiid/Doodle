#pragma once
#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>
namespace doodle {
namespace project_config {
class base_config;

}  // namespace project_config
namespace project_helper {
struct database_t;
}

/**
 * 项目信息类
 */
class DOODLE_CORE_API project : boost::totally_ordered<project> {
 public:
  // 项目名称
  std::string p_name;
  // 项目路径
  FSys::path p_path;
  // 项目全拼
  std::string p_en_str;
  // 项目简拼
  std::string p_shor_str;
  // 项目本机路径
  FSys::path p_local_path;
  // 自动灯光上传路径
  FSys::path p_auto_upload_path;

 public:
  /// 添加entt指示, 用于保持指针稳定性
  static constexpr bool in_place_delete{true};
  project();
  explicit project(FSys::path in_path, std::string in_name);
  explicit project(
      std::string in_name, FSys::path in_path, std::string in_en_str, std::string in_shor_str, FSys::path in_local_path,
      FSys::path in_auto_upload_path
  )
      : p_name(std::move(in_name)),
        p_path(std::move(in_path)),
        p_en_str(std::move(in_en_str)),
        p_shor_str(std::move(in_shor_str)),
        p_local_path(std::move(in_local_path)),
        p_auto_upload_path(std::move(in_auto_upload_path)) {}
  explicit project(const project_helper::database_t& in);

  [[nodiscard]] const std::string& get_name() const;
  void set_name(const std::string& Name) noexcept;

  [[nodiscard]] const FSys::path& get_path() const noexcept;
  [[nodiscard]] FSys::path make_path(const FSys::path& in_path) const;
  void set_path(const FSys::path& Path);

  [[nodiscard]] std::string str() const;
  [[nodiscard]] std::string show_str() const;

  [[nodiscard]] std::string short_str() const;

  bool operator<(const project& in_rhs) const;
  bool operator==(const project& in_rhs) const;

  operator project_helper::database_t() const;

 private:
  friend void to_json(nlohmann::json& j, const project& p) {
    j["name"] = p.p_name;
    j["path"] = p.p_path;
  }
  friend void from_json(const nlohmann::json& j, project& p) {
    j.at("name").get_to(p.p_name);
    j.at("path").get_to(p.p_path);
  }
};

struct project_ptr {
  project* project_;
};

namespace project_helper {

struct database_t {
  std::int32_t id_{};
  uuid uuid_id_{};

  std::string name_{};
  std::filesystem::path path_{};
  std::string en_str_{};
  std::string shor_str_{};
  std::filesystem::path local_path_{};
  std::string auto_upload_path_{};

  friend void to_json(nlohmann::json& j, const database_t& p) {
    j["name"] = p.name_;
    j["path"] = p.path_;
  }
};

};  // namespace project_helper

}  // namespace doodle
