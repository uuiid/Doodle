#pragma once

#include <DoodleLib/DoodleLib_fwd.h>
#include <sqlpp11/mysql/mysql.h>
DOODLE_NAMESPACE_S
/**
 * @brief 这个是sql连接单例， 负责配置生成sql连接
 * 
 */
class DOODLELIB_API CoreSql {
  std::shared_ptr<sqlpp::mysql::connection_config> config;

  explicit CoreSql();

 public:
  DOODLE_DISABLE_COPY(CoreSql)

  void Init();
  [[nodiscard]] static CoreSql& Get();
  [[nodiscard]] ConnPtr getConnection() const;

};

DOODLE_NAMESPACE_E
