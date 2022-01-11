#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
/**
 * @brief 这个是sql连接单例， 负责配置生成sql连接
 *
 */
class DOODLELIB_API core_sql : public details::no_copy {
  class impl;
  std::unique_ptr<impl> p_i;
  explicit core_sql();

 public:
  ~core_sql();

  void Init();
  [[nodiscard]] static core_sql& Get();
  [[nodiscard]] conn_ptr get_connection(const FSys::path& in_path) const;
};

}  // namespace doodle
