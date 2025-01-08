//
// Created by TD on 24-9-24.
//
#include "doodle_lib/core/http/multipart_body.h"
#include <doodle_lib/core/scan_win_service.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

namespace doodle::scan {
std::ostream& boost_test_print_type(std::ostream& ostr, scan_key_t const& right) {
  return ostr << right.name_ << right.number_ << right.version_name_;
}

}  // namespace doodle::scan

BOOST_AUTO_TEST_SUITE(metadata)
using namespace doodle;

/** Read a message from a `std::istream`.

    This function attempts to parse a complete HTTP/1 message from the stream.

    @param is The `std::istream` to read from.

    @param buffer The buffer to use.

    @param msg The message to store the result.

    @param ec Set to the error, if any occurred.
*/
template <class Allocator, bool isRequest, class Body>
void read_istream(
    std::istringstream& is, boost::beast::basic_flat_buffer<Allocator>& buffer,
    boost::beast::http::message<isRequest, Body, boost::beast::http::fields>& msg, boost::beast::error_code& ec
) {
  // Create the message parser
  //
  // Arguments passed to the parser's constructor are
  // forwarded to the message constructor. Here, we use
  // a move construction in case the caller has constructed
  // their message in a non-default way.
  //
  boost::beast::http::parser<isRequest, Body> p{std::move(msg)};

  do {
    // Extract whatever characters are presently available in the istream
    if (is.rdbuf()->in_avail() > 0) {
      // Get a mutable buffer sequence for writing
      auto const b = buffer.prepare(static_cast<std::size_t>(is.rdbuf()->in_avail()));

      // Now get everything we can from the istream
      buffer.commit(static_cast<std::size_t>(is.readsome(reinterpret_cast<char*>(b.data()), b.size())));
    } else if (buffer.size() == 0) {
      // Our buffer is empty and we need more characters,
      // see if we've reached the end of file on the istream
      if (!is.eof()) {
        // Get a mutable buffer sequence for writing
        auto const b = buffer.prepare(1024);

        // Try to get more from the istream. This might block.
        is.read(reinterpret_cast<char*>(b.data()), b.size());

        // If an error occurs on the istream then return it to the caller.
        if (is.fail() && !is.eof()) {
          // We'll just re-use io_error since std::istream has no error_code interface.
          ec = make_error_code(boost::beast::errc::io_error);
          return;
        }

        // Commit the characters we got to the buffer.
        buffer.commit(static_cast<std::size_t>(is.gcount()));
      } else {
        // Inform the parser that we've reached the end of the istream.
        p.put_eof(ec);
        if (ec) return;
        break;
      }
    }

    // Write the data to the parser
    auto const bytes_used = p.put(buffer.data(), ec);

    // This error means that the parser needs additional octets.
    if (ec == boost::beast::http::error::need_more) ec = {};
    if (ec) return;

    // Consume the buffer octets that were actually parsed.
    buffer.consume(bytes_used);
  } while (!p.is_done());

  // Transfer ownership of the message container in the parser to the caller.
  msg = p.release();
}

BOOST_AUTO_TEST_CASE(scan_key_t) {
  scan::scan_key_t l_k1{}, l_k2{};

  BOOST_TEST(l_k1 == l_k2);
}

BOOST_AUTO_TEST_CASE(multipart_body) {
  std::istringstream l_str{R"(POST /foo HTTP/1.1
Content-Length: 68137
Content-Type: multipart/form-data; boundary=---------------------------974767299852498929531610575

-----------------------------974767299852498929531610575
Content-Disposition: form-data; name="description"

一些文本
-----------------------------974767299852498929531610575
Content-Disposition: form-data; name="myFile"; filename="foo.txt"
Content-Type: text/plain

（上传文件 foo.txt 的内容）
-----------------------------974767299852498929531610575--
)"};

  boost::beast::flat_buffer l_buffer{};
  boost::beast::http::request<doodle::http::multipart_body> l_msg{};
  boost::beast::http::vector_body<char> l_body{};
  boost::beast::error_code l_ec{};
  l_msg.body().boundary_ = "---------------------------974767299852498929531610575";
  read_istream(l_str, l_buffer, l_msg, l_ec);

  BOOST_TEST(l_ec);
}

BOOST_AUTO_TEST_SUITE_END()