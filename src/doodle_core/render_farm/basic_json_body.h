//
// Created by td_main on 2023/8/10.
//

#pragma once
#include <boost/beast.hpp>

#include <nlohmann/json.hpp>
namespace doodle::render_farm::detail {
struct basic_json_body {
 public:
  /** The type of container used for the body

              */
  using json_type  = nlohmann::json;
  using value_type = json_type;

  /** Returns the payload size of the body
  */
  static std::uint64_t size(value_type const& body) { return body.dump().size(); }

  /** The algorithm for parsing the body

      Meets the requirements of <em>BodyReader</em>.
  */

  class reader {
    value_type& body_;
    std::string json_str_;

   public:
    template <bool isRequest, class Fields>
    explicit reader(boost::beast::http::header<isRequest, Fields>&, value_type& b) : body_(b) {}

    void init(boost::optional<std::uint64_t> const& length, boost::system::error_code& ec);

    template <class ConstBufferSequence>
    std::size_t put(ConstBufferSequence const& buffers, boost::system::error_code& ec) {
      auto const extra = boost::beast::buffer_bytes(buffers);
      auto const size  = json_str_.size();
      if (extra > json_str_.max_size() - size) {
        BOOST_BEAST_ASSIGN_EC(ec, boost::beast::http::error::buffer_overflow);
        return 0;
      }

      json_str_.resize(size + extra);
      ec         = {};
      char* dest = &json_str_[size];
      for (auto b : boost::beast::buffers_range_ref(buffers)) {
        std::char_traits<char>::copy(dest, static_cast<char const*>(b.data()), b.size());
        dest += b.size();
      }
      return extra;
    }

    void finish(boost::system::error_code& ec);
  };

  /** The algorithm for serializing the body

      Meets the requirements of <em>BodyWriter</em>.
  */

  class writer {
    value_type const& body_;
    std::string json_str_;

   public:
    using const_buffers_type = boost::asio::const_buffer;

    template <bool isRequest, class Fields>
    explicit writer(boost::beast::http::header<isRequest, Fields> const&, value_type const& b) : body_(b) {}

    void init(boost::system::error_code& ec);

    boost::optional<std::pair<const_buffers_type, bool>> get(boost::system::error_code& ec);
  };
};

}  // namespace doodle::render_farm::detail
