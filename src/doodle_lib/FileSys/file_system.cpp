#include <doodle_lib/Exception/exception.h>
#include <doodle_lib/FileSys/file_system.h>
#include <doodle_lib/Logger/logger.h>
#include <date/date.h>

#include <boost/asio.hpp>

namespace doodle {
void file_system::local_copy(const FSys::path& in_sourcePath, const FSys::path& targetPath, bool backup) {
  //创建线程池多线程复制
  boost::asio::thread_pool pool(std::thread::hardware_concurrency());
  //验证文件存在
  // if (FSys::exists(targetPath)) return false;
  if (!FSys::exists(in_sourcePath))
    throw file_error(in_sourcePath, "不存在路径");
  if (!FSys::exists(targetPath.parent_path()))
    FSys::create_directories(targetPath.parent_path());
  FSys::path backup_path{};
  std::string time_str{};
  if (backup) {
    auto time = std::chrono::system_clock::now();

    time_str    = date::format("%Y_%m_%d_%H_%M_%S", time);
    backup_path = targetPath.parent_path() / "backup" / time_str /
                  targetPath.filename();
  }

  if (FSys::is_regular_file(in_sourcePath)) {  //复制文件
    if (!FSys::exists(targetPath.parent_path()))
      FSys::create_directories(targetPath.parent_path());
    boost::asio::post(pool, [=]() {
      FSys::copy_file(in_sourcePath, targetPath,
                      FSys::copy_options::overwrite_existing);
    });

    if (backup) {
      if (!FSys::exists(backup_path.parent_path())) {
        FSys::create_directories(backup_path.parent_path());
      }
      boost::asio::post(pool, [=]() {
        FSys::copy_file(
            in_sourcePath, backup_path,
            FSys::copy_options::overwrite_existing);
      });
    }

  } else {  //复制目录
    auto dregex = std::regex(in_sourcePath.generic_string());
    DOODLE_LOG_INFO(fmt::format("{} --> {}",
                                in_sourcePath.generic_string().c_str(),
                                targetPath.generic_string().c_str()));
    backup_path = targetPath / "backup" / time_str;
    for (auto& item :
         FSys::recursive_directory_iterator(in_sourcePath)) {
      if (FSys::is_regular_file(item.path())) {
        FSys::path basic_string = std::regex_replace(
            item.path().generic_string(), dregex, targetPath.generic_string());
        boost::asio::post(pool, [=]() {
          if (!FSys::exists(basic_string.parent_path()))
            FSys::create_directories(basic_string.parent_path());

          FSys::copy_file(
              item.path(), basic_string,
              FSys::copy_options::overwrite_existing);
        });
        if (backup) {
          FSys::path basic_backup_path = std::regex_replace(
              item.path().generic_string(), dregex, backup_path.generic_string());
          boost::asio::post(pool, [=]() {
            if (!FSys::exists(basic_backup_path.parent_path()))
              FSys::create_directories(basic_backup_path.parent_path());

            FSys::copy_file(
                item.path(), basic_backup_path,
                FSys::copy_options::overwrite_existing);
          });
        }
      }
    }
  }
  pool.join();
}

}  // namespace doodle
