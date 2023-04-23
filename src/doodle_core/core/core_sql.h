#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <rttr/rttr_enable.h>
#include <string_view>
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
  static constexpr std::string_view memory_data{":memory:"};
  FSys::path path_;
  database_info() : database_info(memory_data){};
  explicit database_info(FSys::path in_path) : path_(std::move(in_path)){};

  [[nodiscard]] conn_ptr get_connection() const;
  [[nodiscard]] conn_ptr get_connection_const() const;
};

}  // namespace details

}  // namespace doodle
