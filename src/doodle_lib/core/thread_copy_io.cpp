//
// Created by TD on 2024/1/6.
//

#include "thread_copy_io.h"

#include <doodle_core/logger/logger.h>
namespace doodle {

void thread_copy_io_service::copy_file(const FSys::path &from, const FSys::path &to) const {
  boost::ignore_unused(this);

  if (!FSys::exists(to) || FSys::file_size(from) != FSys::file_size(to) ||
      FSys::last_write_time(from) != FSys::last_write_time(to)) {
    FSys::copy_file(from, to, FSys::copy_options::overwrite_existing);
  }
}

boost::system::error_code thread_copy_io_service::copy_impl(
    const FSys::path &from, const FSys::path &to, FSys::copy_options in_options
) const {
  boost::system::error_code l_ec{};
  try {
    if (!FSys::is_directory(from)) {
      copy_file(from, to);
    }

    if (in_options == FSys::copy_options::recursive) {
      for (auto &&l_file : FSys::recursive_directory_iterator(from)) {
        auto l_to_file = to / l_file.path().lexically_proximate(from);
        if (l_file.is_directory()) {
          FSys::create_directories(l_to_file);
        } else {
          copy_file(l_file.path(), l_to_file);
        }
      }
    } else {
      for (auto &&l_file : FSys::directory_iterator(from)) {
        auto l_to_file = to / l_file.path().lexically_proximate(from);
        if (l_file.is_directory()) {
          FSys::create_directories(l_to_file);
        } else {
          copy_file(l_file.path(), l_to_file);
        }
      }
    }
  } catch (const FSys::filesystem_error &in_error) {
    default_logger_raw()->log(log_loc(), spdlog::level::err, "复制文件错误 {}", in_error.what());
    l_ec = in_error.code();
    BOOST_ASIO_ERROR_LOCATION(l_ec);
  } catch (const std::system_error &in_error) {
    default_logger_raw()->log(log_loc(), spdlog::level::err, in_error.what());
    l_ec = in_error.code();
    BOOST_ASIO_ERROR_LOCATION(l_ec);
  } catch (...) {
    default_logger_raw()->log(
        log_loc(), spdlog::level::err, "未知错误 {}", boost::current_exception_diagnostic_information()
    );
    l_ec = boost::system::errc::make_error_code(boost::system::errc::io_error);
    BOOST_ASIO_ERROR_LOCATION(l_ec);
  }
  return l_ec;
}

}  // namespace doodle