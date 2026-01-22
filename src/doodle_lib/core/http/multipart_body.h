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
#include <cctype>
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
  enum class parser_line_state { begin_parser, header, data, eof_end };

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
    using boundary_searcher_type = decltype(std::boyer_moore_searcher{
        std::begin(std::declval<std::string>()), std::end(std::declval<std::string>())
    });
    using boundary_searcher_ptr  = std::unique_ptr<boundary_searcher_type>;
    boundary_searcher_ptr boundary_searcher_{};

    enum boundary_state {
      not_boundary,   // 边界错误
      boundary,       // 正常的边界
      boundary_end,   // 最后的边界
      need_more_data  // 缓冲区不足, 需要更多数据
    };

    void get_boundary() {
      boundary_.clear();
      boundary_searcher_.reset();

      if (fields_.count(boost::beast::http::field::content_type) == 0) return;

      // e.g. multipart/form-data; boundary=----WebKitFormBoundaryxxx
      //      multipart/form-data; boundary="----WebKitFormBoundaryxxx"
      const auto l_content_type_sv = fields_.at(boost::beast::http::field::content_type);
      const std::string l_content_type{l_content_type_sv.data(), l_content_type_sv.size()};
      std::string l_lower = l_content_type;
      boost::to_lower(l_lower);
      static constexpr std::string_view boundary_key = "boundary=";
      const auto l_pos                               = l_lower.find(boundary_key);
      if (l_pos == std::string::npos) return;

      std::size_t l_begin = l_pos + boundary_key.size();
      // while (l_begin < l_content_type.size() && std::isspace(static_cast<unsigned char>(l_content_type[l_begin]))) {
      //   ++l_begin;
      // }
      // if (l_begin >= l_content_type.size()) return;

      std::size_t l_end   = std::string::npos;
      if (l_content_type[l_begin] == '"') {
        ++l_begin;
        l_end = l_content_type.find('"', l_begin);
        if (l_end == std::string::npos) return;
      } else {
        l_end = l_content_type.find(';', l_begin);
        if (l_end == std::string::npos) l_end = l_content_type.size();
      }
      if (l_end <= l_begin) return;
      boundary_ = l_content_type.substr(l_begin, l_end - l_begin);
      if (boundary_.empty()) return;
      boundary_searcher_ = std::make_unique<boundary_searcher_type>(boundary_searcher_type{
          std::begin(boundary_),
          std::end(boundary_),
      });
    }

   public:
    template <bool isRequest, class Fields>
    explicit reader(boost::beast::http::header<isRequest, Fields>& in_header, value_type& b)
        : body_(b), line_state_(parser_line_state::begin_parser), fields_{in_header} {}
    void init(boost::optional<std::uint64_t> const& length, boost::system::error_code& ec) {
      get_boundary();
      ec = {};
      if (!boundary_searcher_ || boundary_.empty()) {
        BOOST_BEAST_ASSIGN_EC(ec, boost::asio::error::invalid_argument);
        return;
      }
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
          if (!std::holds_alternative<std::string>(part_.body_)) {
            part_.body_ = std::string{in_begin, in_end};
          } else
            std::get<std::string>(part_.body_) += std::string{in_begin, in_end};
          break;
        case detail::content_type::unknown:
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
      if (!boundary_searcher_) {
        BOOST_BEAST_ASSIGN_EC(ec, boost::asio::error::invalid_argument);
        return 0;
      }
      auto const l_extra = boost::beast::buffer_bytes(in_buffers);
      ec                 = {};
      if (l_extra > 4096) {
        BOOST_BEAST_ASSIGN_EC(ec, boost::asio::error::message_size);
        return 0;
      }
      boost::asio::buffer_copy(buffer_.prepare(l_extra), in_buffers);
      buffer_.commit(l_extra);

      // 如果缓存不足以包含五个边界, 则继续等待, 保留现有数据, 继续接收
      if (buffer_.size() < boundary_.size() * 5) {
        return l_extra;
      }
      do {
        auto l_parse_size = paser(ec);
        if (ec) return 0;
        if (l_parse_size == 0) break;
        buffer_.consume(l_parse_size);
        if (buffer_.size() < boundary_.size() * 5) break;
      } while (true);

      return l_extra;
    }

    inline std::size_t paser(boost::system::error_code& ec) {
      std::size_t l_size = 0;
      const static std::boyer_moore_searcher searcher_new_line_{
          std::begin(newline_),
          std::end(newline_),
      };
      /// 使用boyer_moore算法查找边界
      auto l_begin = boost::asio::buffers_begin(buffer_.data()), l_end = boost::asio::buffers_end(buffer_.data());
      switch (line_state_) {
        case parser_line_state::begin_parser: {
          auto&& [l_b, l_begin_b] = is_boundary(l_begin, l_end);
          if (l_b == need_more_data) break;
          if (l_b == not_boundary) {
            ec = boost::asio::error::invalid_argument;
            return 0;
          }
          line_state_ = parser_line_state::header;
          l_size      = boundary_.size() + 2 + 2;  // --边界 + \r\n
          break;
        }
        case parser_line_state::header: {
          auto l_end_r = std::search(l_begin, l_end, searcher_new_line_);
          if (l_end_r == l_end) break;  // 不是完整的一行

          auto l_end_n      = l_end_r != l_end ? l_end_r + 1 : l_end;  // 指向 \r\n 位置中的 \n
          auto l_end_n_next = l_end_n != l_end ? l_end_n + 1 : l_end;  // 指向下一行的开始位置
          l_size            = std::distance(l_begin, l_end_n_next);    // 本次处理的字节数
          if (l_begin == l_end_r)
            line_state_ = parser_line_state::data;  // 空行, 表示头部已经完成解析
          else
            parser_headers(l_begin, l_end_r);
        } break;
        case parser_line_state::data: {
          auto&& [l_b, l_begin_b] = is_boundary(l_begin, l_end);
          switch (l_b) {
            case boundary: {
              // 找到边界, 说明数据结束
              add_data(l_begin, l_begin_b - 2);                                            // 去除换行
              l_size      = std::distance(l_begin, l_begin_b) + boundary_.size() + 2 + 2;  // --边界 + \r\n
              line_state_ = parser_line_state::header;
              parser_part_end();
            } break;
            case boundary_end: {
              // 找到最后的边界, 说明数据结束
              add_data(l_begin, l_begin_b - 2);                                            // 去除换行
              l_size      = std::distance(l_begin, l_begin_b) + boundary_.size() + 2 + 2;  // --边界--
              line_state_ = parser_line_state::eof_end;
              parser_part_end();
            } break;
            case need_more_data:
              // 边界可能跨 buffer, 等待更多数据
              l_size = 0;
              break;
            case not_boundary:;  // 不是边界, 说明是数据
              add_data(l_begin, l_end);
              l_size = buffer_.size();
              break;
          }
        } break;
        case parser_line_state::eof_end:
          break;
      }
      return l_size;
    }

    // 比较边界分隔符
    template <typename Iter>
    std::tuple<boundary_state, Iter> is_boundary(const Iter& in_begin, const Iter& in_end) const {
      BOOST_ASSERT(boundary_searcher_ && !boundary_.empty());

      auto l_find_ = std::search(in_begin, in_end, *boundary_searcher_);
      if (l_find_ == in_end) {
        // 没有找到边界, 说明数据有问题
        return {not_boundary, in_begin};
      }

      // 需要读取 l_find_[-1], l_find_[-2]
      if (std::distance(in_begin, l_find_) < 2) return {not_boundary, in_begin};

      if (*(l_find_ - 1) != '-' || *(l_find_ - 2) != '-') {
        // 边界前面不是 --
        return {not_boundary, in_begin};
      }

      auto l_boundary_end = l_find_ + boundary_.size();

      // 需要读取 l_boundary_end[0], l_boundary_end[1]
      if (std::distance(l_boundary_end, in_end) < 2) return {boundary, l_find_ - 2};

      if (*(l_boundary_end) == '-' && *(l_boundary_end + 1) == '-') {
        return {boundary_end, l_find_ - 2};
      }

      if (*(l_boundary_end) != '\r' || *(l_boundary_end + 1) != '\n') {
        // 边界后面不是换行符, 说明数据有问题
        return {not_boundary, in_begin};
      }

      return {boundary, l_find_ - 2};
    }

    void finish(boost::system::error_code& ec) {
      ec = {};
      if (buffer_.size() > 0) {
        do {
          auto l_parse_size = paser(ec);
          if (ec) return;
          if (l_parse_size == 0) break;
          buffer_.consume(l_parse_size);
        } while (true);
      }
    }
  };
};

}  // namespace doodle::http
