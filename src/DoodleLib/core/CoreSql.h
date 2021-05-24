#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

DOODLE_NAMESPACE_S

class DOODLELIB_API CoreSql {
  std::shared_ptr<sqlpp::mysql::connection_config> config;

  explicit CoreSql();
 public:
  DOODLE_DISABLE_COPY(CoreSql)

  [[nodiscard]] static CoreSql& Get();
  [[nodiscard]] ConnPtr getConnection() const;
};

DOODLE_NAMESPACE_E
