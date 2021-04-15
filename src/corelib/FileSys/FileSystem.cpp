#include <corelib/FileSys/FileSystem>
#include <corelib/Exception/Exception.h>
#include <magic_enum.hpp>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <nlohmann/json.hpp>
#include <loggerlib/Logger.h>
#include <date/date.h>
namespace doodle {
void FileSystem::localCopy(const FSys::path& sourePath, const FSys::path& targetPath, const bool backup) {
  //创建线程池多线程复制
  boost::asio::thread_pool pool(std::thread::hardware_concurrency());
  //验证文件存在
  // if (boost::filesystem::exists(targetPath)) return false;
  if (!boost::filesystem::exists(sourePath)) throw FileError(sourePath, "不存在路径");
  if (!boost::filesystem::exists(targetPath.parent_path()))
    boost::filesystem::create_directories(targetPath.parent_path());
  FSys::path backup_path = "";
  std::string time_str   = "";
  if (backup) {
    auto time = std::chrono::system_clock::now();

    time_str    = date::format("%Y_%m_%d_%H_%M_%S", time);
    backup_path = targetPath.parent_path() / "backup" / time_str /
                  targetPath.filename();
  }

  if (boost::filesystem::is_regular_file(sourePath)) {  //复制文件
    if (!boost::filesystem::exists(targetPath.parent_path()))
      boost::filesystem::create_directories(targetPath.parent_path());
    boost::asio::post(pool, [=]() {
      boost::filesystem::copy_file(sourePath, targetPath,
                                   boost::filesystem::copy_option::overwrite_if_exists);
    });

    if (backup) {
      if (!boost::filesystem::exists(backup_path.parent_path())) {
        boost::filesystem::create_directories(backup_path.parent_path());
      }
      boost::asio::post(pool, [=]() {
        boost::filesystem::copy_file(
            sourePath, backup_path,
            boost::filesystem::copy_option::overwrite_if_exists);
      });
    }

  } else {  //复制目录
    auto dregex = std::regex(sourePath.generic_string());
    DOODLE_LOG_INFO(sourePath.generic_string().c_str()
                    << "-->"
                    << targetPath.generic_string().c_str());
    backup_path = targetPath / "backup" / time_str;
    for (auto& item :
         boost::filesystem::recursive_directory_iterator(sourePath)) {
      if (boost::filesystem::is_regular_file(item.path())) {
        FSys::path basic_string = std::regex_replace(
            item.path().generic_string(), dregex, targetPath.generic_string());
        boost::asio::post(pool, [=]() {
          if (!boost::filesystem::exists(basic_string.parent_path()))
            boost::filesystem::create_directories(basic_string.parent_path());

          boost::filesystem::copy_file(
              item.path(), basic_string,
              boost::filesystem::copy_option::overwrite_if_exists);
        });
        if (backup) {
          FSys::path basic_backup_path = std::regex_replace(
              item.path().generic_string(), dregex, backup_path.generic_string());
          boost::asio::post(pool, [=]() {
            if (!boost::filesystem::exists(basic_backup_path.parent_path()))
              boost::filesystem::create_directories(basic_backup_path.parent_path());

            boost::filesystem::copy_file(
                item.path(), basic_backup_path,
                boost::filesystem::copy_option::overwrite_if_exists);
          });
        }
      }
    }
  }
  pool.join();
}

}  // namespace doodle