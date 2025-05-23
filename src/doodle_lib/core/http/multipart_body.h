//
// Created by TD on 25-1-7.
//

#pragma once
#include "doodle_core/core/core_set.h"

#include <doodle_lib/core/http/http_content_type.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/beast/http.hpp>

#include <string>
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

    nlohmann::json to_json() {
      nlohmann::json l_json{};
      for (auto&& i : parts_) {
        std::visit(
            overloaded{
                [&](const FSys::path&) {},
                [&](const std::string& in_body) {
                  if (i.content_type == detail::content_type::application_json) {
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
    std::vector<FSys::path> get_files() {
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
    }
  };

  // struct value_type {
  //   std::vector<part_value_type> parts_{};
  //   std::string boundary_{};
  // };
  using value_type = value_type_impl;

  class reader {
    value_type& body_;
    part_value_type part_;
    parser_line_state line_state_;
    std::optional<std::size_t> length_{0};
    std::string boundary_{};
    std::function<std::string()> get_boundary_function_{};
    std::optional<std::ofstream> out_file_;

    template <bool isRequest, class Fields>
    static std::string get_boundary(boost::beast::http::header<isRequest, Fields>& in_header) {
      if (in_header.count(boost::beast::http::field::content_type) > 0) {
        const auto& l_content_type = in_header.at(boost::beast::http::field::content_type);
        if (auto l_pos = l_content_type.find("boundary="); l_pos != std::string::npos) {
          return l_content_type.substr(l_pos + 9, l_content_type.find(l_pos, ';'));
        }
      }
      return {};
    }

   public:
    template <bool isRequest, class Fields>
    explicit reader(boost::beast::http::header<isRequest, Fields>& in_header, value_type& b)
        : body_(b), get_boundary_function_{[&in_header]() { return get_boundary(in_header); }} {}
    void init(boost::optional<std::uint64_t> const& length, boost::system::error_code& ec) {
      boundary_   = get_boundary_function_();
      line_state_ = parser_line_state::boundary;
      ec          = {};
      if (length) length_ = *length;
    }
    template <class InIt>
    void parser_headers(InIt const& in_begin, InIt const& in_end) {
      std::string in_header{in_begin, in_end};
      if (auto l_it = in_header.find(':'); l_it != in_header.npos) {
        auto l_c = in_header.substr(0, l_it);
        boost::to_lower(l_c);
        if (l_c == "content-disposition") {
          if (auto l_pos = in_header.find("name="); l_pos != in_header.npos) {
            auto l_end_pos = in_header.find(';', l_pos);
            part_.name     = in_header.substr(l_pos + 6, l_end_pos - (l_pos + 6) - 1);
            if (l_end_pos == in_header.npos) part_.name.pop_back();
          }
          if (auto l_pos = in_header.find("filename="); l_pos != in_header.npos) {
            auto l_end_pos  = in_header.find(';', l_pos);
            part_.file_name = in_header.substr(l_pos + 10, l_end_pos - (l_pos + 10) - 1);
            if (l_end_pos == in_header.npos) part_.file_name.pop_back();
          }
        }
        if (l_c == "content-type") {
          if (auto l_pos = in_header.find(':'); l_pos != in_header.npos) {
            auto l_str         = in_header.substr(l_pos + 2, in_header.find(l_pos, ';'));
            part_.content_type = detail::get_content_type(l_str);
          }
        }
      }
    }

    template <class InIt>
    void add_data(InIt const& in_begin, InIt const& in_end) {
      switch (part_.content_type) {
        case detail::content_type::application_json:
        case detail::content_type::unknown:
          if (!std::holds_alternative<std::string>(part_.body_)) {
            part_.body_ = std::string{in_begin, in_end};
          } else
            std::get<std::string>(part_.body_) += std::string{in_begin, in_end};
          break;
        default:
          if (!std::holds_alternative<FSys::path>(part_.body_)) {
            part_.body_ =
                core_set::get_set().get_cache_root("http") / core_set::get_set().get_uuid_str() / part_.file_name;
            out_file_ =
                std::make_optional<std::ofstream>(std::get<FSys::path>(part_.body_), std::ios::out | std::ios::binary);
          } else {
            (*out_file_) << std::string{in_begin, in_end};
          }
          break;
      }
    }
    void parser_part_end() {
      body_.parts_.emplace_back(std::move(part_));
      part_ = {};
    }
    template <class ConstBufferSequence>
    std::size_t put(ConstBufferSequence const& buffers, boost::system::error_code& ec) {
      // auto const extra       = boost::beast::buffer_bytes(buffers);
      std::size_t l_size = 0;
      ec                 = {};
      // const auto& l_buff = boost::beast::buffers_cat(buffers);
      for (auto l_begin = boost::asio::buffers_begin(buffers), l_end = boost::asio::buffers_end(buffers);
           l_begin != l_end;) {
        decltype(l_begin) l_end_eof = std::find(l_begin, l_end, '\r');
        while (l_end_eof != l_end && *(++l_end_eof) != '\n') l_end_eof = std::find(l_begin, l_end, '\r');
        if (line_state_ != parser_line_state::data && l_end_eof == l_end)
          return l_size;  // 不是完整的一行, 直接返回, 下次解析
        l_size += std::distance(l_begin, l_end_eof == l_end ? l_end : l_end_eof + 1);
        {
          auto l_end_eof_t = l_end_eof;
          --l_end_eof_t;
          switch (line_state_) {
            case parser_line_state::boundary: {
              if (*l_begin != '-') return ec = make_error_code(boost::system::errc::invalid_argument), 0;
              ++ ++l_begin;  // 跳过 --
              std::string l_boundary{l_begin, l_end_eof_t};
              if (l_boundary.size() == boundary_.size() + 2 && l_boundary.substr(0, 2) == "--") {
                line_state_ = parser_line_state::eof_end;
                break;
              }
              if (l_boundary != boundary_) return ec = make_error_code(boost::system::errc::invalid_argument), 0;
              line_state_ = parser_line_state::header;
              break;
            }
            case parser_line_state::header:
              if (l_begin == l_end_eof_t)
                line_state_ = parser_line_state::data;  // 空行, 表示头部已经完成解析
              else
                parser_headers(l_begin, l_end_eof_t);
              break;
            case parser_line_state::data:
              add_data(l_begin, l_end_eof == l_end ? l_end_eof : l_end_eof_t);
              if (*(l_end_eof_t) == '\r' && *(l_end_eof) == '\n') {
                line_state_ = parser_line_state::boundary;  // 数据块已经结束
                parser_part_end();
              }
              break;
            case parser_line_state::eof_end:
              break;
          }
        }
        l_begin = l_end_eof == l_end ? l_end : ++l_end_eof;
      }
      return l_size;
    }
    void finish(boost::system::error_code& ec) { ec = {}; }
  };
};
}  // namespace doodle::http
