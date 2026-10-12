//
// multipart_body_split_boundary_test.cpp — 边界跨 put() 分片的回归测试
//
// 背景：/api/doodle/ai/light/session/{id}/reference 的多图上传会间歇性
// 只回来 1 条参考图（201 成功、静默少一个 part）。真实上传 2 张 1.5MB 的 PNG
// 时约每 10 次命中一次。
//
// 现有 multipart_body_test.cpp 的分块用例（parse_chunked_input）用 128 字节定长
// 切片、数据量很小，且切片落点与真实 TCP 读的落点无关，因此从未覆盖到
// 「boundary 恰好被 put() 的边界切开」这条路径。
//
// 本文件专门把边界串切成两半喂进去，验证解析器不丢 part。
//

#include <doodle_lib/core/http/http_content_type.h>
#include <doodle_lib/core/http/multipart_body.h>
#include <doodle_lib/core/http/multipart_body_value.h>

#include <boost/asio/buffer.hpp>
#include <boost/beast/http.hpp>
#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace {

const std::string kBoundary = "----WebKitFormBoundarySplitTest0123";

/// 构造一个含 2 个文件 part 的 multipart body（模仿 superagent 多图上传）
std::string build_two_file_body(const std::string& in_a, const std::string& in_b) {
  std::string body;
  body += "--" + kBoundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"file\"; filename=\"a.png\"\r\n";
  body += "Content-Type: image/png\r\n";
  body += "\r\n";
  body += in_a;
  body += "\r\n";
  body += "--" + kBoundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"file\"; filename=\"b.png\"\r\n";
  body += "Content-Type: image/png\r\n";
  body += "\r\n";
  body += in_b;
  body += "\r\n";
  body += "--" + kBoundary + "--\r\n";
  return body;
}

/// 按给定的切片点把 body 依次喂进 reader
std::size_t feed_in_slices(
    doodle::http::multipart_body::reader& in_reader, const std::string& in_body,
    const std::vector<std::size_t>& in_cuts, boost::system::error_code& ec
) {
  std::size_t l_offset = 0;
  std::vector<std::size_t> l_points = in_cuts;
  l_points.push_back(in_body.size());
  for (auto l_point : l_points) {
    if (l_point <= l_offset) continue;
    const std::size_t l_len = l_point - l_offset;
    in_reader.put(boost::asio::buffer(in_body.data() + l_offset, l_len), ec);
    if (ec) return l_offset;
    l_offset = l_point;
  }
  return l_offset;
}

/// 在「第 n 个 part 的结束 boundary」中间切一刀
std::vector<std::size_t> cut_middle_of_boundary(const std::string& in_body, std::size_t in_which, std::size_t in_off) {
  const std::string l_delim = "--" + kBoundary;
  std::size_t l_hit = 0;
  std::size_t l_pos = 0;
  while ((l_pos = in_body.find(l_delim, l_pos)) != std::string::npos) {
    if (l_hit == in_which) return {l_pos + in_off};
    ++l_hit;
    l_pos += l_delim.size();
  }
  return {};
}

}  // namespace

BOOST_AUTO_TEST_SUITE(multipart_body_split_boundary)

/// 基线：不切片，2 个 part 应都能解析出来
BOOST_AUTO_TEST_CASE(baseline_two_parts_single_write) {
  const std::string l_body = build_two_file_body(std::string(300, 'A'), std::string(300, 'B'));

  boost::beast::http::request<boost::beast::http::empty_body> l_req;
  l_req.set(boost::beast::http::field::content_type, "multipart/form-data; boundary=" + kBoundary);

  doodle::http::multipart_body::value_type l_value;
  doodle::http::multipart_body::reader l_reader{l_req, l_value};

  boost::system::error_code l_ec;
  l_reader.init(boost::optional<std::uint64_t>(l_body.size()), l_ec);
  BOOST_TEST_REQUIRE(!l_ec);

  l_reader.put(boost::asio::buffer(l_body), l_ec);
  BOOST_TEST_REQUIRE(!l_ec);
  l_reader.finish(l_ec);
  BOOST_TEST_REQUIRE(!l_ec);

  BOOST_CHECK_EQUAL(l_value.parts_.size(), 2u);
  BOOST_CHECK_EQUAL(l_value.get_files().size(), 2u);
}

/// 回归：第 2 个 boundary 被从中间切开时，两个 part 都必须保留
///
/// 这是 /session/{id}/reference 多图上传丢图的直接复现。
BOOST_AUTO_TEST_CASE(boundary_split_across_writes_keeps_all_parts) {
  const std::string l_body = build_two_file_body(std::string(1024, 'A'), std::string(1024, 'B'));

  // 第 1 个 boundary 出现的位置（0 是首 boundary，1 是第 1 个 part 的结束 boundary）
  for (std::size_t l_off = 1; l_off < kBoundary.size() + 2; ++l_off) {
    const auto l_cuts = cut_middle_of_boundary(l_body, 1, l_off);
    BOOST_TEST_REQUIRE(!l_cuts.empty());

    boost::beast::http::request<boost::beast::http::empty_body> l_req;
    l_req.set(boost::beast::http::field::content_type, "multipart/form-data; boundary=" + kBoundary);

    doodle::http::multipart_body::value_type l_value;
    doodle::http::multipart_body::reader l_reader{l_req, l_value};

    boost::system::error_code l_ec;
    l_reader.init(boost::optional<std::uint64_t>(l_body.size()), l_ec);
    BOOST_TEST_REQUIRE(!l_ec);

    feed_in_slices(l_reader, l_body, l_cuts, l_ec);
    if (l_ec) {
      BOOST_TEST_MESSAGE(
          "切片偏移 " << l_off << " (cut=" << l_cuts[0] << ", body=" << l_body.size()
                      << ") put() 报错: " << l_ec.message() << ", 已解析 part 数=" << l_value.parts_.size()
      );
    }
    l_reader.finish(l_ec);
    BOOST_TEST_MESSAGE(
        "切片偏移 " << l_off << " => parts=" << l_value.parts_.size() << " files=" << l_value.get_files().size()
                    << " finish_ec=" << (l_ec ? l_ec.message() : std::string{"<ok>"})
    );
    if (l_ec) l_ec.clear();

    BOOST_TEST_CONTEXT("在 boundary 内偏移 " << l_off << " 处切片") {
      BOOST_CHECK_EQUAL(l_value.parts_.size(), 2u);
      BOOST_CHECK_EQUAL(l_value.get_files().size(), 2u);
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
