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

    auto l_to   = to;
    auto l_from = from;
    static FSys::path l_path{R"(\\?\)"};
    if (l_to.make_preferred().native().size() > MAX_PATH) {
      l_to = l_path / l_to;
    }
    if (l_from.make_preferred().native().size() > MAX_PATH) {
      l_from = l_path / l_from;
    }

    FSys::copy_file(l_from, l_to, FSys::copy_options::overwrite_existing);
  }
}

void thread_copy_io_service::copy_old_file(const FSys::path &from, const FSys::path &to) {
  if (!FSys::exists(to) || FSys::file_size(from) != FSys::file_size(to) ||
      FSys::last_write_time(from) != FSys::last_write_time(to)) {
    if (!FSys::exists(to) || FSys::last_write_time(from) > FSys::last_write_time(to)) {
      if (!FSys::exists(to.parent_path())) FSys::create_directories(to.parent_path());

      auto l_to   = to;
      auto l_from = from;
      static FSys::path l_path{R"(\\?\)"};
      if (l_to.make_preferred().native().size() > MAX_PATH) {
        l_to = l_path / l_to;
      }
      if (l_from.make_preferred().native().size() > MAX_PATH) {
        l_from = l_path / l_from;
      }

      FSys::copy_file(l_from, l_to, FSys::copy_options::overwrite_existing);
    }
  }
}

boost::system::error_code thread_copy_io_service::copy_impl(
    const FSys::path &from, const FSys::path &to, FSys::copy_options in_options, logger_ptr in_logger,
    copy_fun_ptr in_fun_ptr
) const {
  boost::system::error_code l_ec{};
  std::error_code l_error_code_NETNAME_DELETED{ERROR_NETNAME_DELETED, std::system_category()};
  for (int i = 0; i < 10; ++i) {
    try {
      in_logger->log(log_loc(), spdlog::level::warn, "复制 {} -> {}", from, to);
      if (FSys::is_regular_file(from) && !FSys::is_hidden(from) &&
          from.extension() != doodle_config::doodle_flag_name) {
        in_fun_ptr(from, to);
        return l_ec;
      }

      if (in_options == FSys::copy_options::recursive) {
        for (auto &&l_file : FSys::recursive_directory_iterator(from)) {
          auto l_to_file = to / l_file.path().lexically_proximate(from);
          if (l_file.is_regular_file() && !FSys::is_hidden(l_file.path()) &&
              l_file.path().extension() != doodle_config::doodle_flag_name) {
            in_fun_ptr(l_file.path(), l_to_file);
          }
        }
      } else {
        for (auto &&l_file : FSys::directory_iterator(from)) {
          auto l_to_file = to / l_file.path().lexically_proximate(from);
          if (l_file.is_regular_file() && !FSys::is_hidden(l_file.path()) &&
              l_file.path().extension() != doodle_config::doodle_flag_name) {
            in_fun_ptr(l_file.path(), l_to_file);
          }
        }
      }
      return l_ec;
    } catch (const FSys::filesystem_error &in_error) {
      if (in_error.code() == l_error_code_NETNAME_DELETED) {
        in_logger->log(log_loc(), spdlog::level::warn, "复制文件网络错误 开始重试第 {}次 {}, ", i++, in_error.what());
      } else {
        in_logger->log(log_loc(), spdlog::level::err, "复制文件错误 {}", in_error.what());
        l_ec = in_error.code();
        BOOST_ASIO_ERROR_LOCATION(l_ec);
        break;
      }
    } catch (const std::system_error &in_error) {
      in_logger->log(log_loc(), spdlog::level::err, in_error.what());
      l_ec = in_error.code();
      BOOST_ASIO_ERROR_LOCATION(l_ec);
      break;
    } catch (...) {
      in_logger->log(log_loc(), spdlog::level::err, "未知错误 {}", boost::current_exception_diagnostic_information());
      l_ec = boost::system::errc::make_error_code(boost::system::errc::io_error);
      BOOST_ASIO_ERROR_LOCATION(l_ec);
      break;
    }
  }

  return l_ec;
}

boost::system::error_code thread_copy_io_service::delete_impl(
    const std::vector<FSys::path> &from, const FSys::path &to, const std::vector<FSys::path> &in_exclude_local_dir,
    FSys::copy_options in_options, logger_ptr in_logger
) const {
  boost::system::error_code l_ec{};
  std::error_code l_error_code_NETNAME_DELETED{ERROR_NETNAME_DELETED, std::system_category()};
  for (int i = 0; i < 10; ++i) {
    try {
      in_logger->log(log_loc(), spdlog::level::warn, "删除 {} 中不存在, {} 中存在的文件", from, to);

      std::vector<FSys::path> l_remove_files{};
      if (in_options == FSys::copy_options::recursive) {
        for (auto &&l_file : FSys::recursive_directory_iterator(to)) {
          auto l_lex_path = l_file.path().lexically_proximate(to);

          if (std::any_of(
                  in_exclude_local_dir.begin(), in_exclude_local_dir.end(),
                  [&l_lex_path](const FSys::path &in_exclude) {
                    return l_lex_path.generic_string().starts_with(in_exclude.generic_string());
                  }
              )) {
            continue;
          }

          if (std::none_of(from.begin(), from.end(), [&l_lex_path](const FSys::path &l_from) {
                return FSys::exists(l_from / l_lex_path);
              })) {
            l_remove_files.emplace_back(l_file.path());
          }
        }

      } else {
        for (auto &&l_file : FSys::directory_iterator(to)) {
          auto l_lex_path = l_file.path().lexically_proximate(to);
          if (std::any_of(
                  in_exclude_local_dir.begin(), in_exclude_local_dir.end(),
                  [&l_lex_path](const FSys::path &in_exclude) {
                    return l_lex_path.generic_string().starts_with(in_exclude.generic_string());
                  }
              )) {
            continue;
          }

          if (std::none_of(from.begin(), from.end(), [&l_lex_path](const FSys::path &l_from) {
                return FSys::exists(l_from / l_lex_path);
              })) {
            l_remove_files.emplace_back(l_file.path());
          }
        }
      }

      for (auto &&l_file : l_remove_files) {
        FSys::remove_all(l_file);
      }

      return l_ec;
    } catch (const FSys::filesystem_error &in_error) {
      if (in_error.code() == l_error_code_NETNAME_DELETED) {
        in_logger->log(log_loc(), spdlog::level::warn, "复制文件网络错误 开始重试第 {}次 {}, ", i++, in_error.what());
      } else {
        in_logger->log(log_loc(), spdlog::level::err, "复制文件错误 {}", in_error.what());
        l_ec = in_error.code();
        BOOST_ASIO_ERROR_LOCATION(l_ec);
        break;
      }
    } catch (const std::system_error &in_error) {
      in_logger->log(log_loc(), spdlog::level::err, in_error.what());
      l_ec = in_error.code();
      BOOST_ASIO_ERROR_LOCATION(l_ec);
      break;
    } catch (...) {
      in_logger->log(log_loc(), spdlog::level::err, "未知错误 {}", boost::current_exception_diagnostic_information());
      l_ec = boost::system::errc::make_error_code(boost::system::errc::io_error);
      BOOST_ASIO_ERROR_LOCATION(l_ec);
      break;
    }
  }

  return l_ec;
}

}  // namespace doodle