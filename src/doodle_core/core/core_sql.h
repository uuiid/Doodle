#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <string_view>
#include <utility>

namespace doodle {

namespace details {
/**
 * @brief 这个是sql连接类, 负责配置和生成sql链接
 *
 */
class DOODLE_CORE_API database_info {
 public:
  static constexpr std::string_view memory_data{":memory:"};
  FSys::path path_;
  database_info() : database_info(memory_data){};
  explicit database_info(FSys::path in_path) : path_(std::move(in_path)){};

  [[nodiscard]] conn_ptr get_connection() const;
  [[nodiscard]] conn_ptr get_connection_const() const;
};

class DOODLE_CORE_API database_pool_info {
  void create_pool(const std::string& in_path);
  std::shared_ptr<sqlpp::sqlite3::connection_pool> pool_;
  FSys::path path_;

 public:
  static constexpr std::string_view memory_data{":memory:"};

  database_pool_info() : database_pool_info(memory_data){};
  explicit database_pool_info(FSys::path in_path) : path_(std::move(in_path)) {
    create_pool(in_path.generic_string());
  };

  // path
  inline void set_path(const FSys::path& in_path) {
    path_ = in_path;
    create_pool(in_path.generic_string());
  }
  [[nodiscard]] inline const FSys::path& get_path() const { return path_; }

  [[nodiscard]] pooled_connection get_connection() const;
};

}  // namespace details

}  // namespace doodle
