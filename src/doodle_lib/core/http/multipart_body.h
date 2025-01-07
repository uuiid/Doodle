//
// Created by TD on 25-1-7.
//

#pragma once
#include <doodle_lib/core/http/http_session_data.h>

#include <boost/asio/ssl/error.hpp>
#include <boost/beast/http.hpp>

#include <string>
namespace doodle::http {
struct multipart_body {
  struct part_value_type {
    std::string name{};
    std::string file_name{};
    detail::content_type content_type{};
  };
  enum class parser_state {
    uninitialized = 1,
    start,
    start_boundary,
    header_field_start,
    header_field,
    headers_almost_done,
    header_value_start,
    header_value,
    header_value_almost_done,
    part_data_start,
    part_data,
    part_data_almost_boundary,
    part_data_boundary,
    part_data_almost_end,
    part_data_end,
    part_data_final_hyphen,
    end
  };

  struct value_type {
    std::vector<part_value_type> parts_{};
    std::string boundary_{};
  };
  // using value_type = std::vector<part_value_type>;

  class reader {
    value_type& body_;
    part_value_type part_;
    parser_state state_;
    std::size_t index_{0};
    std::string header_field_;
    std::string header_value_;
    std::optional<std::size_t> length_{0};
    std::size_t pos_{};

   public:
    template <bool isRequest, class Fields>
    explicit reader(boost::beast::http::header<isRequest, Fields>&, value_type& b) : body_(b), state_() {}
    void init(boost::optional<std::uint64_t> const& length, boost::system::error_code& ec) {
      state_ = parser_state::start;
      index_ = 0;
      ec     = {};
      if (length) length_ = *length;
    }
    void parser_headers() {}
    void add_data(const char& input) {}

    template <class ConstBufferSequence>
    std::size_t put(ConstBufferSequence const& buffers, boost::system::error_code& ec) {
      auto const extra   = boost::beast::buffer_bytes(buffers);
      ec                 = {};
      const auto& l_buff = boost::beast::buffers_range_ref(buffers);
      for (auto l_begin = l_buff.begin(), l_end = l_buff.end(); l_begin != l_end; ++l_begin) {
        switch (state_) {
          case parser_state::start:
            index_ = 0;
            state_ = parser_state::start_boundary;
          case parser_state::start_boundary:
            if (index_ == 0 && (*l_begin == '-' && *(l_begin + 1) == '-')) {
              ++(++l_begin);  // 跳过 --
            }

            if (index_ == body_.boundary_.size()) {
              if (*l_begin != '\r') return ec = make_error_code(boost::system::errc::invalid_argument), 0;
              ++index_;
              break;
            }
            if (index_ == body_.boundary_.size() + 1) {
              if (*l_begin != '\n') return ec = make_error_code(boost::system::errc::invalid_argument), 0;
              index_ = 0;
              state_ = parser_state::header_field_start;
            }

            if (*l_begin != body_.boundary_[index_++])
              return ec = make_error_code(boost::system::errc::invalid_argument), 0;
            break;
          case parser_state::header_field_start:
            state_ = parser_state::header_field;
          case parser_state::header_field:
            if (*l_begin == '\r') {
              state_ = parser_state::headers_almost_done;
              break;
            }
            if (*l_begin == ' ' || *l_begin == '\t' || *l_begin == ':') {
              state_ = parser_state::header_value_start;
              break;
            }
            if (*l_begin == '-') break;
            auto bl = std::tolower(*l_begin);
            if (bl != '-' && (bl < 'a' || bl > 'z') && (bl < '0' || bl > '9'))
              return ec = make_error_code(boost::system::errc::invalid_argument), 0;
            header_field_ += *l_begin;
            break;

          case parser_state::headers_almost_done:
            if (*l_begin == '\n') return ec = make_error_code(boost::system::errc::invalid_argument), 0;
            state_ = parser_state::part_data_start;
            break;
          case parser_state::header_value_start:
            if (*l_begin == ' ' || *l_begin == '\t') break;
            state_ = parser_state::header_value;
          case parser_state::header_value:
            if (*l_begin == '\r') {
              state_ = parser_state::header_value_almost_done;
              break;
            }
            header_value_ += *l_begin;
            break;
          case parser_state::header_value_almost_done:
            if (*l_begin != '\n') return ec = make_error_code(boost::system::errc::invalid_argument), 0;
            state_ = parser_state::header_field_start;
            break;
          case parser_state::part_data_start:
            state_ = parser_state::part_data;
            parser_headers();
          case parser_state::part_data:
            if (*l_begin == '\r') {
              state_ = parser_state::part_data_almost_boundary;
            }
            add_data(*l_begin);
            break;
          case parser_state::part_data_almost_boundary:
            if (*l_begin != '\n') return ec = make_error_code(boost::system::errc::invalid_argument), 0;
            state_ = parser_state::part_data_boundary;
            index_ = 0;
            ++ ++l_begin;
            break;
          case parser_state::part_data_boundary:
            if (*l_begin == body_.boundary_[index_]) {
              state_ = parser_state::part_data;
              break;
            }
            if (++index_ == body_.boundary_.size()) {
              state_ = parser_state::part_data_almost_end;
              break;
            }

          case parser_state::part_data_almost_end:
            if (*l_begin == '-') {
              state_ = parser_state::part_data_final_hyphen;
              break;
            }
            if (*l_begin == '\r') {
              state_ = parser_state::part_data_end;
              break;
            }
          case parser_state::part_data_final_hyphen:
            if (*l_begin != '-') return ec = make_error_code(boost::system::errc::invalid_argument), 0;
            state_ = parser_state::end;
            break;
          case parser_state::part_data_end:
            if (*l_begin != '\n') return ec = make_error_code(boost::system::errc::invalid_argument), 0;
            state_ = parser_state::header_field_start;
            break;
          case parser_state::end:
            break;

          default:
            return ec = make_error_code(boost::system::errc::invalid_argument), 0;
        }
        ++pos_;
      }
      return extra;
    }
  };
};
}  // namespace doodle::http
