#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <rttr/rttr_enable.h>
#include <utility>

namespace doodle {

namespace details {
/**
 * @brief 这个是sql连接类, 负责配置和生成sql链接
 *
 */
class DOODLE_CORE_API database_info {
  RTTR_ENABLE();

 public:
  FSys::path path_;
  database_info() : database_info(":memory:"){};
  explicit database_info(FSys::path in_path) : path_(std::move(in_path)){};

  [[nodiscard]] conn_ptr get_connection() const;
  [[nodiscard]] conn_ptr get_connection_const() const;
};

}  // namespace details

using database_info = entt::locator<details::database_info>;
}  // namespace doodle
