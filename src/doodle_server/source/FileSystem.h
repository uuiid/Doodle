#pragma once

#include <doodle_server/DoodleServer_global.h>
#include <vector>
#include <mutex>
#include <fstream>

DOODLE_NAMESPACE_S
class IoFile : public std::enable_shared_from_this<IoFile> {
 public:
  IoFile(std::shared_ptr<FSys::path> path);
  ~IoFile();

  bool read(char* buffer, uint64_t size, uint64_t offset);
  bool write(char* buffer, uint64_t size, uint64_t offset);

 private:
  friend class FileSystem;

  std::mutex p_mutex;
  std::shared_ptr<FSys::path> p_path;
  std::fstream p_file;
};

class FileSystem {
 public:
  FileSystem(const FileSystem&) = delete;
  FileSystem& operator=(const FileSystem&) = delete;

  static FileSystem& Get() noexcept;

  std::shared_ptr<IoFile> open(const std::shared_ptr<FSys::path>& path);
  bool rename(const FSys::path* source, const FSys::path* target);
  bool copy(const FSys::path* source, const FSys::path* target);

 private:
  friend class IoFile;
  FileSystem();
  std::mutex p_mutex;
  std::vector<IoFile*> p_fm;
};

DOODLE_NAMESPACE_E