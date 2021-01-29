#include <doodle_server/source/FileSystem.h>

// #include <sstream>
#include <iostream>
#include <boost/filesystem.hpp>
// #include <boost/iostreams/stream.hpp>
// #include <boost/iostreams/device/mapped_file.hpp>
DOODLE_NAMESPACE_S
FileSystem& FileSystem::Get() noexcept {
  static FileSystem k_instance;
  return k_instance;
}

std::shared_ptr<IoFile> FileSystem::open(const std::shared_ptr<fileSys::path>& path) {
  std::unique_lock lock{p_mutex};  //加锁

  auto file = std::find_if(p_fm.begin(), p_fm.end(),
                           [=](const IoFile* f) {
                             return f->p_path == path;
                           });

  if (file != p_fm.end()) {
    return (*file)->shared_from_this();
  } else {
    if (!fileSys::exists(*path)) {
      fileSys::fstream f{*path, std::ios::out | std::ios::binary};
    }
    auto k_p = std::make_shared<IoFile>(path);
    p_fm.push_back(k_p.get());
    return k_p;
  }
}

bool FileSystem::rename(const fileSys::path* source, const fileSys::path* target) {
  std::unique_lock lock{p_mutex};  //加锁
  auto file = std::find_if(p_fm.begin(), p_fm.end(),
                           [=](const IoFile* f) {
                             return f->p_path.get() == source;
                           });
  if (file == p_fm.end()) {
    if (!fileSys::exists(target->parent_path())) {
      fileSys::create_directories(target->parent_path());
    }
    if (fileSys::exists(*source)) {
      fileSys::rename(*source, *target);
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

FileSystem::FileSystem()
    : p_mutex(),
      p_fm() {
}

IoFile::IoFile(std::shared_ptr<fileSys::path> path)
    : p_mutex(),
      p_path(std::move(path)),
      p_file() {
  if (!p_file.is_open()) {
    p_file.open(p_path->generic_string(), std::ios::out | std::ios::in | std::ios::binary);
  }
}

IoFile::~IoFile() {
  auto& f = FileSystem::Get();
  {
    std::unique_lock lock{f.p_mutex};
    f.p_fm.erase(std::find(f.p_fm.begin(), f.p_fm.end(), this));
  }
}

bool IoFile::read(char* buffer, uint64_t size, uint64_t offset) {
  {
    std::unique_lock lock{p_mutex};  //加锁 , std::try_to_lock
    if (lock) {
      p_file.seekg(offset, std::ios::beg);

      p_file.read(buffer, size);

    } else {
      return false;
    }
  }

  return p_file.good();
}

bool IoFile::write(char* buffer, uint64_t size, uint64_t offset) {
  {
    std::unique_lock lock{p_mutex};  //加锁 , std::try_to_lock
    if (lock) {
      p_file.seekp(offset, std::ios::beg);
      p_file.write(buffer, size);
    } else {
      return false;
    }
  }
  return p_file.good();
}

DOODLE_NAMESPACE_E
