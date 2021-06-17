#pragma once

#include <DoodleLib/DoodleLib_fwd.h>


namespace doodle {
/**
 * @brief 这个是sql连接单例， 负责配置生成sql连接
 * 
 */
class DOODLELIB_API CoreSql :public details::no_copy{
  std::shared_ptr<sqlpp::mysql::connection_config> config;

  explicit CoreSql();

 public:

  void Init();
  [[nodiscard]] static CoreSql& Get();
  [[nodiscard]] ConnPtr getConnection() const;

};

}
