//
// Created by TD on 2024/1/6.
//

#include "thread_copy_io.h"

#include <doodle_core/logger/logger.h>
namespace doodle {

void thread_copy_io_service::copy_file(const FSys::path &from, const FSys::path &to) {
  if (!FSys::exists(to) || FSys::file_size(from) != FSys::file_size(to) ||
      FSys::last_write_time(from) != FSys::last_write_time(to)) {
    if (!FSys::exists(to.parent_path())) FSys::create_directories(to.parent_path());
    FSys::copy_file(from, to, FSys::copy_options::overwrite_existing);
  }
}

void thread_copy_io_service::copy_old_file(const FSys::path &from, const FSys::path &to) {
  if (!FSys::exists(to) || FSys::file_size(from) != FSys::file_size(to) ||
      FSys::last_write_time(from) != FSys::last_write_time(to)) {
    if (!FSys::exists(to) || FSys::last_write_time(from) > FSys::last_write_time(to)) {
      if (!FSys::exists(to.parent_path())) FSys::create_directories(to.parent_path());
      FSys::copy_file(from, to, FSys::copy_options::overwrite_existing);
    }
  }
}

boost::system::error_code thread_copy_io_service::copy_impl(
    const FSys::path &from, const FSys::path &to, FSys::copy_options in_options, copy_fun_ptr in_fun_ptr
) const {
  boost::system::error_code l_ec{};
  try {
    default_logger_raw()->log(log_loc(), spdlog::level::info, "复制 {} -> {}", from, to);
    if (FSys::is_regular_file(from)) {
      in_fun_ptr(from, to);
      return l_ec;
    }

    if (in_options == FSys::copy_options::recursive) {
      for (auto &&l_file : FSys::recursive_directory_iterator(from)) {
        auto l_to_file = to / l_file.path().lexically_proximate(from);
        if (l_file.is_regular_file()) {
          in_fun_ptr(l_file.path(), l_to_file);
        }
      }
    } else {
      for (auto &&l_file : FSys::directory_iterator(from)) {
        auto l_to_file = to / l_file.path().lexically_proximate(from);
        if (l_file.is_regular_file()) {
          in_fun_ptr(l_file.path(), l_to_file);
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