//
// Created by TD on 25-1-7.
//

#pragma once
#include "doodle_core/core/core_set.h"

#include <doodle_lib/core/http/http_content_type.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/beast/core/buffers_cat.hpp>
#include <boost/beast/core/buffers_suffix.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http.hpp>

#include <iterator>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <sys/stat.h>

namespace doodle::http {

struct multipart_body {
  struct part_value_type {
    std::string name{};
    std::string file_name{};
    detail::content_type content_type{detail::content_type::unknown};
    std::variant<std::string, FSys::path> body_{};
  };
  enum class parser_line_state { boundary, header, data, eof_end };

  struct value_type_impl {
    std::vector<part_value_type> parts_{};

    nlohmann::json to_json() const {
      nlohmann::json l_json{};
      for (auto&& i : parts_) {
        std::visit(
            overloaded{
                [&](const FSys::path&) {},
                [&](const std::string& in_body) {
                  if (nlohmann::json::accept(in_body)) {
                    l_json[i.name] = nlohmann::json::parse(in_body);
                  } else
                    l_json[i.name] = in_body;
                },
            },
            i.body_
        );
      }
      return l_json;
    }
    std::vector<FSys::path> get_files() const {
      std::vector<FSys::path> l_result{};
      for (auto&& i : parts_) {
        std::visit(
            overloaded{
                [&](const FSys::path& in_path) { l_result.emplace_back(in_path); },
                [&](const std::string&) {},
            },
            i.body_
        );
      }
      return l_result;
    }
  };

  // struct value_type {
  //   std::vector<part_value_type> parts_{};
  //   std::string boundary_{};
  // };
  using value_type = value_type_impl;

  class reader {
    enum state {
      s_uninitialized = 1,
      s_start,
      s_start_boundary,
      s_header_field_start,
      s_header_field,
      s_headers_almost_done,
      s_header_value_start,
      s_header_value,
      s_header_value_almost_done,
      s_part_data_start,
      s_part_data,
      s_part_data_almost_boundary,
      s_part_data_boundary,
      s_part_data_almost_end,
      s_part_data_end,
      s_part_data_final_hyphen,
      s_end
    };
    struct multipart_parser_settings;

    struct multipart_parser {
      void* data;

      size_t index;
      size_t boundary_length;

      unsigned char state;

      const multipart_parser_settings* settings;

      std::string lookbehind;
      std::string multipart_boundary;

      explicit multipart_parser(const std::string& boundary, const multipart_parser_settings* settings)
          : index(0),
            boundary_length(boundary.length()),
            state(s_start),
            settings(settings),
            lookbehind(boundary.length() + 8, '\0'),
            multipart_boundary(boundary) {}
    };
    typedef int (*multipart_data_cb)(multipart_parser*, std::string_view);
    typedef int (*multipart_notify_cb)(multipart_parser*);
    struct multipart_parser_settings {
      multipart_data_cb on_header_field;
      multipart_data_cb on_header_value;
      multipart_data_cb on_part_data;

      multipart_notify_cb on_part_data_begin;
      multipart_notify_cb on_headers_complete;
      multipart_notify_cb on_part_data_end;
      multipart_notify_cb on_body_end;
    };
    static constexpr char CR = '\r';
    static constexpr char LF = '\n';
#define NOTIFY_CB(FOR)                     \
  do {                                     \
    if (p->settings->on_##FOR) {           \
      if (p->settings->on_##FOR(p) != 0) { \
        return i;                          \
      }                                    \
    }                                      \
  } while (0)

    // #define EMIT_DATA_CB(FOR, ptr, len)                                            \
    //   do {                                                                         \
    //     if (p->settings->on_##FOR) {                                               \
    //       if (p->settings->on_##FOR(p, std::string_view{ptr, ptr + (len)}) != 0) { \
    //         return i;                                                              \
    //       }                                                                        \
    //     }                                                                          \
    //   } while (0)

#define EMIT_DATA_CB(FOR, ptr, len)   \
  do {                                \
    if (on_##FOR(p, ptr, len) != 0) { \
      return i;                       \
    }                                 \
  } while (0)
    // #define EMIT_DATA_CB(FOR, ptr, len) ;

    void multipart_log(const char* fmt, ...) {}
    template <typename Iterator>
    size_t multipart_parser_execute(multipart_parser* p, Iterator buf, size_t len);

    value_type& body_;
    part_value_type part_;
    std::string boundary_{};
    std::optional<std::ofstream> out_file_;
    boost::beast::http::fields& fields_;
    std::shared_ptr<multipart_parser> parser_;
    multipart_parser_settings settings_{};

    enum boundary_state {
      boundary_error,  // 边界错误
      boundary,        // 正常的边界
      boundary_end,    // 最后的边界
    };

    void get_boundary() {
      if (fields_.count(boost::beast::http::field::content_type) > 0) {
        const auto& l_content_type = fields_.at(boost::beast::http::field::content_type);
        if (auto l_pos = l_content_type.find("boundary="); l_pos != std::string::npos) {
          boundary_ = l_content_type.substr(l_pos + 9, l_content_type.find(l_pos, ';'));
        }
      }
    }
    template <typename Iterator>
    int on_header_field(multipart_parser* p, Iterator in_begin, std::size_t in_length) {
      std::string l_field{in_begin, in_begin + in_length};
      spdlog::debug("on_header_field: {}", l_field);
      auto* l_reader = static_cast<reader*>(p->data);
      boost::algorithm::to_lower(l_field);
      if (l_field == "Content-Disposition") {
        l_reader->part_ = part_value_type{};
      }
      return 0;
    }
    template <typename Iterator>
    int on_header_value(multipart_parser* p, Iterator in_begin, std::size_t in_length) {
      std::string l_value{in_begin, in_begin + in_length};
      spdlog::debug("on_header_value: {}", l_value);
      auto* l_reader = static_cast<reader*>(p->data);
      boost::algorithm::to_lower(l_value);
      if (l_value.find("multipart/form-data") != std::string::npos) {
        auto l_end = l_value.find(";");
        // 解析 name 和 filename
        std::vector<std::string> l_parts;
        boost::algorithm::split(l_parts, l_value, boost::is_any_of(";"));
        for (auto&& l_part : l_parts) {
          boost::algorithm::trim(l_part);
          if (l_part.find("name=") == 0) {
            auto l_name = l_part.substr(5);
            boost::algorithm::trim_if(l_name, boost::is_any_of("\""));
            l_reader->part_.name = l_name;
          } else if (l_part.find("filename=") == 0) {
            auto l_filename = l_part.substr(9);
            boost::algorithm::trim_if(l_filename, boost::is_any_of("\""));
            l_reader->part_.file_name = l_filename;
          }
        }
      } else if (l_value.find("content-type") != std::string::npos) {
        auto l_type = l_value.substr(13);
        boost::algorithm::trim(l_type);
        l_reader->part_.content_type = detail::get_content_type(l_type);
      }
      return 0;
    }
    template <typename Iterator>
    int on_part_data(multipart_parser* p, Iterator in_begin, std::size_t in_length) {
      auto* l_reader = static_cast<reader*>(p->data);
      spdlog::debug("on_part_data: {} bytes", in_length);
      if (!l_reader->out_file_) {
        if (!l_reader->part_.file_name.empty()) {
          auto l_tmp_path =
              core_set::get_set().get_cache_root("http") /
              (core_set::get_set().get_uuid_str() + FSys::path{l_reader->part_.file_name}.extension().string());
          l_reader->out_file_.emplace(l_tmp_path, std::ios::binary);
          l_reader->part_.body_ = l_tmp_path;
        } else {
          l_reader->part_.body_ = std::string{};
        }
      }
      if (l_reader->out_file_) {
        // l_reader->out_file_->write(in_view.data(), in_view.length());
      } else {
        std::get<std::string>(l_reader->part_.body_).append(in_begin, in_begin + in_length);
      }
      return 0;
    }

   public:
    template <bool isRequest, class Fields>
    explicit reader(boost::beast::http::header<isRequest, Fields>& in_header, value_type& b)
        : body_(b), fields_{in_header} {}
    void init(boost::optional<std::uint64_t> const& length, boost::system::error_code& ec) {
      get_boundary();
      ec            = {};
      parser_       = std::make_shared<multipart_parser>(boundary_, &settings_);
      parser_->data = this;
    }

    template <class ConstBufferSequence>
    std::size_t put(ConstBufferSequence const& buffers, boost::system::error_code& ec) {
      auto const l_extra = boost::beast::buffer_bytes(buffers);
      auto l_buff        = boost::beast::buffers_cat(buffers);
      std::size_t l_size_all{};

      const std::size_t l_size = multipart_parser_execute(parser_.get(), boost::asio::buffers_begin(buffers), l_extra);

      ec                       = {};

      return l_size_all;
    }

    void finish(boost::system::error_code& ec) { ec = {}; }
  };
};

template <typename Iterator>
size_t multipart_body::reader::multipart_parser_execute(multipart_parser* p, Iterator buf, size_t len) {
  size_t i    = 0;
  size_t mark = 0;
  char c, cl;
  int is_last = 0;

  while (i < len) {
    c       = *buf;
    is_last = (i == (len - 1));
    switch (p->state) {
      case s_start:
        multipart_log("s_start");
        p->index = 0;
        p->state = s_start_boundary;

      /* fallthrough */
      case s_start_boundary:
        multipart_log("s_start_boundary");
        if (p->index == p->boundary_length) {
          if (c != CR) {
            return i;
          }
          p->index++;
          break;
        } else if (p->index == (p->boundary_length + 1)) {
          if (c != LF) {
            return i;
          }
          p->index = 0;
          NOTIFY_CB(part_data_begin);
          p->state = s_header_field_start;
          break;
        }
        if (i < 2) {
          break;  // first '--'
        }
        if (c != p->multipart_boundary[p->index]) {
          return i;
        }
        p->index++;
        break;

      case s_header_field_start:
        multipart_log("s_header_field_start");
        mark     = i;
        p->state = s_header_field;

      /* fallthrough */
      case s_header_field:
        multipart_log("s_header_field");
        if (c == CR) {
          p->state = s_headers_almost_done;
          break;
        }

        if (c == ' ' || c == '\t') {
          p->state = s_header_value_start;
          break;
        }

        if (c == '-') {
          break;
        }

        if (c == ':') {
          EMIT_DATA_CB(header_field, buf + mark, i - mark);
          p->state = s_header_value_start;
          break;
        }

        cl = tolower(c);
        if ((c != '-') && (cl < 'a' || cl > 'z')) {
          multipart_log("invalid character in header name");
          return i;
        }
        if (is_last) EMIT_DATA_CB(header_field, buf + mark, (i - mark) + 1);
        break;

      case s_headers_almost_done:
        multipart_log("s_headers_almost_done");
        if (c != LF) {
          return i;
        }

        p->state = s_part_data_start;
        break;

      case s_header_value_start:
        multipart_log("s_header_value_start");
        if (c == ' ' || c == '\t') {
          break;
        }

        mark     = i;
        p->state = s_header_value;

      /* fallthrough */
      case s_header_value:
        multipart_log("s_header_value");
        if (c == CR) {
          EMIT_DATA_CB(header_value, buf + mark, i - mark);
          p->state = s_header_value_almost_done;
          break;
        }
        if (is_last) EMIT_DATA_CB(header_value, buf + mark, (i - mark) + 1);
        break;

      case s_header_value_almost_done:
        multipart_log("s_header_value_almost_done");
        if (c != LF) {
          return i;
        }
        p->state = s_header_field_start;
        break;

      case s_part_data_start:
        multipart_log("s_part_data_start");
        NOTIFY_CB(headers_complete);
        mark     = i;
        p->state = s_part_data;

      /* fallthrough */
      case s_part_data:
        multipart_log("s_part_data");
        if (c == CR && i >= len - p->boundary_length - 6) {
          EMIT_DATA_CB(part_data, buf + mark, i - mark);
          mark             = i;
          p->state         = s_part_data_almost_boundary;
          p->lookbehind[0] = CR;
          break;
        }
        if (is_last) EMIT_DATA_CB(part_data, buf + mark, (i - mark) + 1);
        break;

      case s_part_data_almost_boundary:
        multipart_log("s_part_data_almost_boundary");
        if (c == LF) {
          p->state = s_part_data_boundary;
          i += 2;  // first '--'
          p->lookbehind[1] = LF;
          p->index         = 0;
          break;
        }
        EMIT_DATA_CB(part_data, p->lookbehind.data(), 1);
        p->state = s_part_data;
        mark     = i--;
        break;

      case s_part_data_boundary:
        multipart_log("s_part_data_boundary");
        if (p->multipart_boundary[p->index] != c) {
          EMIT_DATA_CB(part_data, p->lookbehind.data(), 2 + p->index);
          p->state = s_part_data;
          mark     = i--;
          break;
        }
        p->lookbehind[2 + p->index] = c;
        if ((++p->index) == p->boundary_length) {
          NOTIFY_CB(part_data_end);
          p->state = s_part_data_almost_end;
        }
        break;

      case s_part_data_almost_end:
        multipart_log("s_part_data_almost_end");
        if (c == '-') {
          p->state = s_part_data_final_hyphen;
          break;
        }
        if (c == CR) {
          p->state = s_part_data_end;
          break;
        }
        return i;

      case s_part_data_final_hyphen:
        multipart_log("s_part_data_final_hyphen");
        if (c == '-') {
          NOTIFY_CB(body_end);
          p->state = s_end;
          break;
        }
        return i;

      case s_part_data_end:
        multipart_log("s_part_data_end");
        if (c == LF) {
          p->state = s_header_field_start;
          NOTIFY_CB(part_data_begin);
          break;
        }
        return i;

      case s_end:
        multipart_log("s_end: %02X", (int)c);
        break;

      default:
        multipart_log("Multipart parser unrecoverable error");
        return 0;
    }
    ++i;
    ++buf;
  }

  return len;
}

}  // namespace doodle::http
