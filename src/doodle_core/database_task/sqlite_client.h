//
// Created by TD on 2022/6/2.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle::database_n {
class sqlite_client {
 public:
  void open_sqlite(const FSys::path& in_path, bool only_ctx = false);
  void update_entt();
  void create_sqlite();
};

class file_translator {
 public:
  virtual ~file_translator()                                  = default;
  virtual void open(const FSys::path& in_path, bool only_ctx) = 0;
  virtual void save(const FSys::path& in_path)                = 0;
};

class sqlite_file : public file_translator {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  sqlite_file();
  explicit sqlite_file(registry_ptr in_registry);
  virtual ~sqlite_file();

  sqlite_file(const sqlite_file& in) noexcept            = delete;
  sqlite_file& operator=(const sqlite_file& in) noexcept = delete;

  sqlite_file(sqlite_file&& in) noexcept;
  sqlite_file& operator=(sqlite_file&& in) noexcept;

  virtual void open(const FSys::path& in_path, bool only_ctx) override;
  virtual void save(const FSys::path& in_path) override;
};

}  // namespace doodle::database_n
