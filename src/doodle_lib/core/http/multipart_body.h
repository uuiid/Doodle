//
// Created by TD on 25-1-7.
//

#pragma once
#include "doodle_core/core/core_set.h"

#include <doodle_lib/core/http/http_content_type.h>
#include <doodle_lib/core/http/multipart_body_value.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/beast/core/buffers_cat.hpp>
#include <boost/beast/core/buffers_suffix.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/core/static_buffer.hpp>
#include <boost/beast/http.hpp>

#include "core/http/multipart_body_value.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <utility>

namespace doodle::http {

struct multipart_body {
  // struct value_type {
  //   std::vector<part_value_type> parts_{};
  //   std::string boundary_{};
  // };
  using value_type      = multipart_body_impl::value_type_impl;
  using part_value_type = multipart_body_impl::part_value_type;
  enum class parser_line_state { boundary, header, data, eof_end };

  // 换行符
  static constexpr std::array<char, 2> newline_{'\r', '\n'};

  class reader {
    value_type& body_;
    part_value_type part_;
    parser_line_state line_state_;
    std::optional<std::size_t> length_{0};
    std::string boundary_{};
    std::optional<std::ofstream> out_file_;
    boost::beast::http::fields& fields_;
    boost::beast::static_buffer<4096> buffer_;

    enum boundary_state {
      not_boundary,  // 边界错误
      boundary,      // 正常的边界
      boundary_end,  // 最后的边界
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
    std::size_t put(ConstBufferSequence const& in_buffers, boost::system::error_code& ec) {
      auto const l_extra = boost::beast::buffer_bytes(in_buffers);
      std::size_t l_size = 0;
      ec                 = {};
      if (l_extra > 4096) {
        BOOST_BEAST_ASSIGN_EC(ec, boost::asio::error::message_size);
        return 0;
      }

      if (l_extra < boundary_.size() + 4 + 2) {
        // 传入的缓冲区小于分隔符的情况下, 我们直接复制, 不进行解析, 保留到下一次解析
        boost::asio::buffer_copy(buffer_.prepare(l_extra), in_buffers);
        buffer_.commit(l_extra);
        return l_extra;
      }
      const static std::boyer_moore_searcher searcher_new_line_{
          std::begin(newline_),
          std::end(newline_),
      };
      const auto l_my_buffers_size = buffer_.size();
      auto l_new_buffers           = boost::beast::buffers_cat(buffer_.cdata(), in_buffers);
      const auto l_begin = boost::asio::buffers_begin(l_new_buffers), l_end = boost::asio::buffers_end(l_new_buffers);
      // l_end_r 指向 \r\n 位置中的 \r
      auto l_end_r = std::search(l_begin, l_end, searcher_new_line_);

      if (l_end_r == l_end) {  // 不是完整的一行
        if (line_state_ == parser_line_state::data) {
          add_data(l_begin, l_end);
          return l_extra;
        }
        return ec = boost::asio::error::invalid_argument, 0;
      }
      auto l_end_n      = l_end_r != l_end ? l_end_r + 1 : l_end;  // 指向 \r\n 位置中的 \n
      auto l_end_n_next = l_end_n != l_end ? l_end_n + 1 : l_end;  // 指向下一行的开始位置
      l_size            = std::distance(l_begin, l_end_n_next);    // 本次处理的字节数
      // SPDLOG_INFO("解析行 字节数 {} l_end_r={:d}, l_end_n={:d}", l_size, *l_end_r, *l_end_n);
      {
        switch (line_state_) {
          case parser_line_state::boundary:
            switch (is_boundary(l_begin, l_end_r)) {
              case not_boundary:;  // 边界错误
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
            if (l_begin == l_end_r)
              line_state_ = parser_line_state::data;  // 空行, 表示头部已经完成解析
            else
              parser_headers(l_begin, l_end_r);
            break;
          case parser_line_state::data:
            switch (is_boundary(l_begin, l_end_r)) {
              case not_boundary:;  // 不是边界, 说明是数据
                add_data(l_begin, l_end_n_next);
                break;
              case boundary:
                line_state_ = parser_line_state::header;
                parser_part_end();
                break;
              case boundary_end:
                line_state_ = parser_line_state::eof_end;
                parser_part_end();
            }
            break;
          case parser_line_state::eof_end:
            break;
        }
      }
      buffer_.consume(l_size);  // 清空缓存
      l_size = l_size > l_my_buffers_size ? l_size - l_my_buffers_size : 0;
      return l_size;
    }

    // 比较边界分隔符
    template <typename Iter>
    boundary_state is_boundary(const Iter& in_begin, const Iter& in_end) const {
      auto l_len = std::distance(in_begin, in_end);
      if (l_len == boundary_.size()) {
        if (std::equal(in_begin, in_end, boundary_.begin())) {
          return boundary;
        }
      } else if (l_len == boundary_.size() + 2) {
        if (std::equal(in_begin + 2, in_end, boundary_.begin()) && *(in_begin) == '-' && *(in_begin + 1) == '-') {
          return boundary;
        }
      } else if (l_len == boundary_.size() + 4) {
        if (std::equal(in_begin + 2, in_end - 2, boundary_.begin()) && *(in_begin) == '-' && *(in_begin + 1) == '-' &&
            *(in_end - 2) == '-' && *(in_end - 1) == '-') {
          return boundary_end;
        }
      }
      return not_boundary;
    }

    void finish(boost::system::error_code& ec) { ec = {}; }
  };
};

}  // namespace doodle::http
