#include <doodle_server/source/FileSystem.h>

// #include <sstream>
#include <iostream>
// #include <boost/filesystem.hpp>
// #include <boost/iostreams/stream.hpp>
// #include <boost/iostreams/device/mapped_file.hpp>
#include <loggerlib/Logger.h>
#include <regex>
DOODLE_NAMESPACE_S
FileSystem& FileSystem::Get() noexcept {
  static FileSystem k_instance;
  return k_instance;
}

std::shared_ptr<IoFile> FileSystem::open(const std::shared_ptr<FSys::path>& path) {
  std::unique_lock lock{p_mutex};  //加锁

  auto file = std::find_if(p_fm.begin(), p_fm.end(),
                           [=](const IoFile* f) {
                             return f->p_path == path;
                           });

  if (file != p_fm.end()) {
    return (*file)->shared_from_this();
  } else {
    if (!FSys::exists(*path)) {
      if (!FSys::exists(path->parent_path()))
        FSys::create_directories(path->parent_path());
      FSys::fstream f{*path, std::ios::out | std::ios::binary};
    }
    auto k_p = std::make_shared<IoFile>(path);
    p_fm.push_back(k_p.get());
    return k_p;
  }
}

bool FileSystem::rename(const FSys::path* source, const FSys::path* target) {
  std::vector<doodle::IoFile*>::iterator file{};
  {
    std::unique_lock lock{p_mutex};  //加锁
    file = std::find_if(p_fm.begin(), p_fm.end(),
                        [=](const IoFile* f) {
                          return f->p_path.get() == source;
                        });
  }
  if (file != p_fm.end()) return false;
  if (!FSys::exists(*source)) return false;

  if (!FSys::exists(target->parent_path())) {
    FSys::create_directories(target->parent_path());
  }

  try {
    FSys::rename(*source, *target);
  } catch (FSys::filesystem_error& err) {
    DOODLE_LOG_WARN(err.what());
    return false;
  }
  return true;
}

bool FileSystem::copy(const FSys::path* source, const FSys::path* target) {
  std::vector<doodle::IoFile*>::iterator file{};
  {
    std::unique_lock lock{p_mutex};  //加锁
    file = std::find_if(p_fm.begin(), p_fm.end(),
                        [=](const IoFile* f) {
                          return f->p_path.get() == source;
                        });
  }
  if (file == p_fm.end()) {
    if (!FSys::exists(target->parent_path())) {
      FSys::create_directories(target->parent_path());
    }
    if (FSys::exists(*source)) {
      if (FSys::is_regular_file(*source)) {
        FSys::copy_file(*source, *target, FSys::copy_options::overwrite_existing);
      } else {
        auto regex = std::regex(source->generic_string());

        for (auto&& iter : FSys::recursive_directory_iterator(*source)) {
          if (FSys::is_regular_file(iter.path())) {
            FSys::path k_target = std::regex_replace(
                iter.path().generic_string(), regex, target->generic_string());

            if (!FSys::exists(k_target.parent_path())) {
              FSys::create_directories(k_target.parent_path());
            }

            FSys::copy_file(iter.path(), k_target, FSys::copy_options::overwrite_existing);
          }
        }
      }
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

IoFile::IoFile(std::shared_ptr<FSys::path> path)
    : p_mutex(),
      p_path(std::move(path)),
      p_file() {
  if (!p_file.is_open()) {
    p_file = boost::filesystem::fstream(*p_path, std::ios::out | std::ios::in | std::ios::binary);
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
    // std::cout << "bad: " << p_file.bad()
    //           << " eof: " << p_file.eof()
    //           << " fail: " << p_file.fail()
    //           << " id open: " << p_file.is_open()
    //           << std::endl;
  }
  return p_file.good();
}

DOODLE_NAMESPACE_E
