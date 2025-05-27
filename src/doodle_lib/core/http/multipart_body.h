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
    value_type& body_;
    part_value_type part_;
    parser_line_state line_state_;
    std::optional<std::size_t> length_{0};
    std::string boundary_{};
    std::optional<std::ofstream> out_file_;
    boost::beast::http::fields& fields_;

    std::string buffer_;

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

   public:
    template <bool isRequest, class Fields>
    explicit reader(boost::beast::http::header<isRequest, Fields>& in_header, value_type& b)
        : body_(b), line_state_(parser_line_state::boundary), fields_{in_header} {}
    void init(boost::optional<std::uint64_t> const& length, boost::system::error_code& ec) {
      get_boundary();
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
    /**
     * @brief 为了数据的完整性, 会传入数据和 \r\n, 需要在解析时去掉 \r\n(在二进制数据中可能会有 \r\n, 需要特殊处理)
     * @tparam InIt 传入的迭代器
     * @param in_begin 开始位置
     * @param in_end 结束位置
     */
    template <class InIt>
    void add_data(InIt const& in_begin, InIt const& in_end) {
      switch (part_.content_type) {
        case detail::content_type::application_json:
        case detail::content_type::unknown:
          if (!std::holds_alternative<std::string>(part_.body_)) {
            part_.body_ = std::string{in_begin, in_end};
          } else
            std::get<std::string>(part_.body_) += std::string{in_begin, in_end - 2};  // 去掉 \r\n
          break;
        default:
          if (!std::holds_alternative<FSys::path>(part_.body_)) {
            auto l_path =
                core_set::get_set().get_cache_root("http") / core_set::get_set().get_uuid_str() / part_.file_name;
            part_.body_ = l_path;
            if (auto l_p = l_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
            out_file_ = std::make_optional<std::ofstream>(l_path, std::ios::out | std::ios::binary);
          }
          (*out_file_) << std::string{in_begin, in_end};
          break;
      }
    }
    void parser_part_end() {
      body_.parts_.emplace_back(std::move(part_));
      part_     = {};
      out_file_ = {};
    }
    template <class ConstBufferSequence>
    std::size_t put(ConstBufferSequence const& buffers, boost::system::error_code& ec) {
      // auto const l_extra = boost::beast::buffer_bytes(buffers);
      std::size_t l_size = 0;
      ec                 = {};
      // buffer_.
      // boost::beast::buffer
      const auto& l_buff = boost::beast::buffers_cat(boost::asio::buffer(buffer_), buffers);
      auto l_begin = boost::asio::buffers_begin(l_buff), l_end = boost::asio::buffers_end(l_buff);
      decltype(l_begin) l_end_eof = std::find(l_begin, l_end, '\r');
      while (l_end_eof != l_end && *++l_end_eof != '\n') l_end_eof = std::find(l_end_eof, l_end, '\r');
      if (line_state_ != parser_line_state::data && l_end_eof == l_end)
        return buffer_ = {l_begin, l_end}, boost::beast::buffer_bytes(buffers);  // 不是完整的一行, 直接返回, 下次解析
      l_size = std::distance(l_begin, l_end_eof == l_end ? l_end : l_end_eof + 1) - buffer_.size();  // +1 是为了包含 \n
      {
        auto l_end_eof_t = l_end_eof;
        --l_end_eof_t;
        switch (line_state_) {
          case parser_line_state::boundary:
            switch (is_boundary(l_begin, l_end_eof_t)) {
              case boundary_error:;  // 边界错误
                return ec = boost::asio::error::invalid_argument, 0;
              case boundary:
                line_state_ = parser_line_state::header;  // 解析到边界, 进入头部解析状态
                break;
              case boundary_end:
                line_state_ = parser_line_state::eof_end;  // 最后一个边界, 进入结束状态
                break;
            }
            break;
          case parser_line_state::header:
            if (l_begin == l_end_eof_t)
              line_state_ = parser_line_state::data;  // 空行, 表示头部已经完成解析
            else
              parser_headers(l_begin, l_end_eof_t);
            break;
          case parser_line_state::data:
            if (is_boundary(l_begin, l_end_eof_t))
              return line_state_ = parser_line_state::boundary, parser_part_end(),
                     0;  // 是边界分隔符, 说明数据已经结束, 需要解析到下一个边界, 在下一次循环中解析
            add_data(l_begin, l_end_eof == l_end ? l_end : ++l_end_eof);
            // l_size = boost::beast::buffer_bytes(buffers);
            break;
          case parser_line_state::eof_end:
            break;
        }
      }
      buffer_.clear();
      return l_size;
    }

    // 比较边界分隔符
    template <typename Iter>
    boundary_state is_boundary(const Iter& in_begin, const Iter& in_end) const {
      std::int64_t l_index{-2};
      auto l_max_size = static_cast<std::int64_t>(boundary_.size());
      for (auto l_it = in_begin; l_it != in_end; ++l_it, ++l_index) {
        switch (l_index) {
          case -2:
          case -1:
            if (*l_it != '-') return boundary_error;  // 前两个个字符必须是 -
            break;
          default:
            if (l_index >= l_max_size) {
              if (*l_it != '-') return boundary_error;             // 最后两个个字符必须是 -
              if (l_index == l_max_size + 1) return boundary_end;  // 最后一个字符是 - , 说明是最后的边界
            } else {
              if (*l_it != boundary_[l_index]) return boundary_error;  // 后续字符必须与边界分隔符一致
            }
        }
      }
      return boundary;
    }

    void finish(boost::system::error_code& ec) { ec = {}; }
  };
};
}  // namespace doodle::http
