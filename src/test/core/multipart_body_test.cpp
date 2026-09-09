//
// Created by TD on 26-9-9.
//
// multipart_body 解析器测试 — 验证 multipart/form-data 请求解析
// 基于 HAR 文件: E:\cache\down\添加评论.har
// URL: http://192.168.20.89:50026/api/pictures/preview-files/01a084e2-9a70-7313-8070-b5196f069fc2
//

#include <doodle_lib/core/http/http_content_type.h>
#include <doodle_lib/core/http/multipart_body.h>
#include <doodle_lib/core/http/multipart_body_value.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/beast/http.hpp>
#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace {

/// 测试文件路径 — HAR 中上传的 mp4 文件
constexpr auto kTestFilePath = R"(E:\cache\down\sadsadasd.mp4)";

/// HAR 中使用的 boundary
const std::string kBoundary  = "----WebKitFormBoundaryOF08K2bYuVv1X82Z";

/// 构建 multipart/form-data 请求体 (不含 HTTP 头部)
/// @param in_file_content 文件二进制内容
/// @return 完整的 multipart 请求体字节
std::string build_multipart_body(const std::string& in_file_content) {
  std::string body;
  // 初始 boundary
  body += "--" + kBoundary + "\r\n";
  // Content-Disposition 头部
  body += "Content-Disposition: form-data; name=\"file\"; filename=\"sadsadasd.mp4\"\r\n";
  // Content-Type 头部
  body += "Content-Type: video/mp4\r\n";
  // 空行: 头部结束
  body += "\r\n";
  // 文件内容
  body += in_file_content;
  // 文件内容后的 \r\n (multipart 规范要求 boundary 前有换行)
  body += "\r\n";
  // 结束 boundary
  body += "--" + kBoundary + "--\r\n";
  return body;
}

/// 检查文件是否存在
bool file_exists(const std::filesystem::path& in_path) {
  std::error_code ec;
  bool ok = std::filesystem::exists(in_path, ec);
  if (!ok || ec) {
    BOOST_TEST_MESSAGE("File not found: " << in_path.generic_string());
  }
  return ok && !ec;
}

}  // namespace

BOOST_AUTO_TEST_SUITE(multipart_body)

/// 测试 1: 解析带有单个文件字段的 multipart 请求
/// 模拟 HAR 中的 preview-files 上传请求
BOOST_AUTO_TEST_CASE(parse_single_file_upload) {
  BOOST_TEST_REQUIRE(file_exists(kTestFilePath));

  // 读取测试文件
  auto file_size = std::filesystem::file_size(kTestFilePath);
  BOOST_TEST_MESSAGE("Test file: " << kTestFilePath << " (" << file_size << " bytes)");
  std::ifstream file{kTestFilePath, std::ios::binary};
  BOOST_TEST_REQUIRE(file.is_open());
  std::string file_content(file_size, '\0');
  file.read(file_content.data(), static_cast<std::streamsize>(file_size));
  BOOST_CHECK_EQUAL(static_cast<std::size_t>(file.gcount()), file_size);

  // 构建完整的 multipart 请求体
  std::string body = build_multipart_body(file_content);
  BOOST_TEST_MESSAGE("Multipart body size: " << body.size() << " bytes");

  // 创建带有 Content-Type 的 HTTP 请求头部
  boost::beast::http::request<boost::beast::http::empty_body> req;
  req.set(boost::beast::http::field::content_type, "multipart/form-data; boundary=" + kBoundary);

  // 创建 reader 并解析
  doodle::http::multipart_body::value_type body_value;
  doodle::http::multipart_body::reader reader{req, body_value};

  boost::system::error_code ec;
  reader.init(boost::optional<std::uint64_t>(body.size()), ec);
  BOOST_TEST_REQUIRE(!ec);

  // 将数据喂给解析器
  std::size_t consumed = reader.put(boost::asio::buffer(body), ec);
  BOOST_TEST_REQUIRE(!ec);
  BOOST_CHECK_EQUAL(consumed, body.size());

  reader.finish(ec);
  BOOST_TEST_REQUIRE(!ec);

  // 验证解析结果
  BOOST_CHECK_EQUAL(body_value.parts_.size(), 1u);
  if (body_value.parts_.size() >= 1) {
    auto& part = body_value.parts_[0];

    // 验证字段名
    BOOST_CHECK_EQUAL(part.name, "file");

    // 验证文件名
    BOOST_CHECK_EQUAL(part.file_name, "sadsadasd.mp4");

    // 验证 Content-Type 被解析为 video_mp4
    BOOST_CHECK(part.content_type == doodle::http::detail::content_type::video_mp4);

    // 验证 body 是文件路径 (非 application_json 类型会写入文件)
    BOOST_CHECK(std::holds_alternative<std::filesystem::path>(part.body_));
    if (std::holds_alternative<std::filesystem::path>(part.body_)) {
      auto& file_path = std::get<std::filesystem::path>(part.body_);
      BOOST_TEST_MESSAGE("Parsed file saved to: " << file_path.generic_string());
      BOOST_CHECK(std::filesystem::exists(file_path));

      // 验证文件大小一致
      auto parsed_size = std::filesystem::file_size(file_path);
      BOOST_CHECK_EQUAL(parsed_size, file_size);
    }
  }
}

/// 测试 2: 解析带有 JSON 字段的 multipart 请求
/// 验证文本/JSON 字段正确解析为字符串
BOOST_AUTO_TEST_CASE(parse_json_field) {
  const std::string boundary = "----TestBoundary123";
  std::string body;
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"json_field\"\r\n";
  body += "Content-Type: application/json\r\n";
  body += "\r\n";
  body += R"({"key":"value","number":42})";
  body += "\r\n";
  body += "--" + boundary + "--\r\n";

  boost::beast::http::request<boost::beast::http::empty_body> req;
  req.set(boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary);

  doodle::http::multipart_body::value_type body_value;
  doodle::http::multipart_body::reader reader{req, body_value};

  boost::system::error_code ec;
  reader.init(boost::optional<std::uint64_t>(body.size()), ec);
  BOOST_TEST_REQUIRE(!ec);

  std::size_t consumed = reader.put(boost::asio::buffer(body), ec);
  BOOST_TEST_REQUIRE(!ec);
  BOOST_CHECK_EQUAL(consumed, body.size());

  reader.finish(ec);
  BOOST_TEST_REQUIRE(!ec);

  BOOST_CHECK_EQUAL(body_value.parts_.size(), 1u);
  if (body_value.parts_.size() >= 1) {
    auto& part = body_value.parts_[0];
    BOOST_CHECK_EQUAL(part.name, "json_field");
    BOOST_CHECK(part.content_type == doodle::http::detail::content_type::application_json);
    BOOST_CHECK(std::holds_alternative<std::string>(part.body_));
    if (std::holds_alternative<std::string>(part.body_)) {
      BOOST_CHECK_EQUAL(std::get<std::string>(part.body_), R"({"key":"value","number":42})");
    }
  }
}

/// 测试 3: 解析多个部分的 multipart 请求
/// 验证多个字段 (文件 + JSON) 的混合解析
BOOST_AUTO_TEST_CASE(parse_multiple_parts) {
  const std::string boundary = "----MultiPartBoundary";
  std::string body;
  // 第一部分: JSON
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"metadata\"\r\n";
  body += "Content-Type: application/json\r\n";
  body += "\r\n";
  body += R"({"status":"ok"})";
  body += "\r\n";
  // 第二部分: 文件 (小二进制数据)
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"attachment\"; filename=\"test.bin\"\r\n";
  body += "Content-Type: application/octet-stream\r\n";
  body += "\r\n";
  body += std::string("\x00\x01\x02\x03\x04\x05\x06\x07", 8);
  body += "\r\n";
  // 结束
  body += "--" + boundary + "--\r\n";

  boost::beast::http::request<boost::beast::http::empty_body> req;
  req.set(boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary);

  doodle::http::multipart_body::value_type body_value;
  doodle::http::multipart_body::reader reader{req, body_value};

  boost::system::error_code ec;
  reader.init(boost::optional<std::uint64_t>(body.size()), ec);
  BOOST_TEST_REQUIRE(!ec);

  std::size_t consumed = reader.put(boost::asio::buffer(body), ec);
  BOOST_TEST_REQUIRE(!ec);
  BOOST_CHECK_EQUAL(consumed, body.size());

  reader.finish(ec);
  BOOST_TEST_REQUIRE(!ec);

  BOOST_CHECK_EQUAL(body_value.parts_.size(), 2u);

  if (body_value.parts_.size() >= 2) {
    // 第一部分: JSON
    BOOST_CHECK_EQUAL(body_value.parts_[0].name, "metadata");
    BOOST_CHECK(body_value.parts_[0].content_type == doodle::http::detail::content_type::application_json);
    BOOST_CHECK(std::holds_alternative<std::string>(body_value.parts_[0].body_));

    // 第二部分: 文件
    BOOST_CHECK_EQUAL(body_value.parts_[1].name, "attachment");
    BOOST_CHECK_EQUAL(body_value.parts_[1].file_name, "test.bin");
    BOOST_CHECK(std::holds_alternative<std::filesystem::path>(body_value.parts_[1].body_));
    if (std::holds_alternative<std::filesystem::path>(body_value.parts_[1].body_)) {
      auto& file_path = std::get<std::filesystem::path>(body_value.parts_[1].body_);
      BOOST_CHECK(std::filesystem::exists(file_path));
      BOOST_CHECK_EQUAL(std::filesystem::file_size(file_path), 8u);
    }
  }
}

/// 测试 4: 解析不带引号的 name/filename
/// 验证 parser_headers 对不带引号值的处理
BOOST_AUTO_TEST_CASE(parse_unquoted_headers) {
  const std::string boundary = "----UnquotedBoundary";
  std::string body;
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=plain_field; filename=plain.txt\r\n";
  body += "Content-Type: text/plain\r\n";
  body += "\r\n";
  body += "hello world";
  body += "\r\n";
  body += "--" + boundary + "--\r\n";

  boost::beast::http::request<boost::beast::http::empty_body> req;
  req.set(boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary);

  doodle::http::multipart_body::value_type body_value;
  doodle::http::multipart_body::reader reader{req, body_value};

  boost::system::error_code ec;
  reader.init(boost::optional<std::uint64_t>(body.size()), ec);
  BOOST_TEST_REQUIRE(!ec);

  std::size_t consumed = reader.put(boost::asio::buffer(body), ec);
  BOOST_TEST_REQUIRE(!ec);

  reader.finish(ec);
  BOOST_TEST_REQUIRE(!ec);

  BOOST_CHECK_EQUAL(body_value.parts_.size(), 1u);
  if (body_value.parts_.size() >= 1) {
    auto& part = body_value.parts_[0];
    // 不带引号的 name 和 filename 应正确提取
    BOOST_CHECK_EQUAL(part.name, "plain_field");
    BOOST_CHECK_EQUAL(part.file_name, "plain.txt");
  }
}

/// 测试 5: 解析带引号的 boundary 的 Content-Type
/// 验证 boundary 被引号包围时正确提取
BOOST_AUTO_TEST_CASE(parse_quoted_boundary) {
  const std::string boundary = "----QuotedBoundary";
  std::string body;
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"test\"\r\n";
  body += "\r\n";
  body += "data";
  body += "\r\n";
  body += "--" + boundary + "--\r\n";

  boost::beast::http::request<boost::beast::http::empty_body> req;
  // boundary 使用引号包围
  req.set(boost::beast::http::field::content_type, "multipart/form-data; boundary=\"" + boundary + "\"");

  doodle::http::multipart_body::value_type body_value;
  doodle::http::multipart_body::reader reader{req, body_value};

  boost::system::error_code ec;
  reader.init(boost::optional<std::uint64_t>(body.size()), ec);
  BOOST_TEST_REQUIRE(!ec);

  std::size_t consumed = reader.put(boost::asio::buffer(body), ec);
  BOOST_TEST_REQUIRE(!ec);

  reader.finish(ec);
  BOOST_TEST_REQUIRE(!ec);

  BOOST_CHECK_EQUAL(body_value.parts_.size(), 1u);
}

/// 测试 6: 空文件上传
/// 验证空文件内容能正确解析
BOOST_AUTO_TEST_CASE(parse_empty_file) {
  const std::string boundary = "----EmptyFileBoundary";
  std::string body;
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"file\"; filename=\"empty.dat\"\r\n";
  body += "Content-Type: application/octet-stream\r\n";
  body += "\r\n";
  // 空文件内容 (仅 boundary 前的 \r\n)
  body += "\r\n";
  body += "--" + boundary + "--\r\n";

  boost::beast::http::request<boost::beast::http::empty_body> req;
  req.set(boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary);

  doodle::http::multipart_body::value_type body_value;
  doodle::http::multipart_body::reader reader{req, body_value};

  boost::system::error_code ec;
  reader.init(boost::optional<std::uint64_t>(body.size()), ec);
  BOOST_TEST_REQUIRE(!ec);

  std::size_t consumed = reader.put(boost::asio::buffer(body), ec);
  BOOST_TEST_REQUIRE(!ec);

  reader.finish(ec);
  BOOST_TEST_REQUIRE(!ec);

  BOOST_CHECK_EQUAL(body_value.parts_.size(), 1u);
  if (body_value.parts_.size() >= 1) {
    BOOST_CHECK_EQUAL(body_value.parts_[0].file_name, "empty.dat");
    BOOST_CHECK(std::holds_alternative<std::filesystem::path>(body_value.parts_[0].body_));
    if (std::holds_alternative<std::filesystem::path>(body_value.parts_[0].body_)) {
      auto& file_path = std::get<std::filesystem::path>(body_value.parts_[0].body_);
      BOOST_CHECK(std::filesystem::exists(file_path));
      BOOST_CHECK_EQUAL(std::filesystem::file_size(file_path), 0u);
    }
  }
}

/// 测试 7: 大块数据分块输入
/// 验证 parser 正确处理跨多次 put() 调用的数据
BOOST_AUTO_TEST_CASE(parse_chunked_input) {
  const std::string boundary = "----ChunkedBoundary";
  std::string body;
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"chunked\"\r\n";
  body += "Content-Type: application/json\r\n";
  body += "\r\n";
  // 构造一个较大的 JSON 字符串 (超过 boundary * 5)
  std::string json_data = "{\"data\":\"";
  json_data += std::string(2000, 'x');  // 填充数据
  json_data += "\"}";
  body += json_data;
  body += "\r\n";
  body += "--" + boundary + "--\r\n";

  boost::beast::http::request<boost::beast::http::empty_body> req;
  req.set(boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary);

  doodle::http::multipart_body::value_type body_value;
  doodle::http::multipart_body::reader reader{req, body_value};

  boost::system::error_code ec;
  reader.init(boost::optional<std::uint64_t>(body.size()), ec);
  BOOST_TEST_REQUIRE(!ec);

  // 分多次小块输入
  const std::size_t chunk_size = 128;
  std::size_t total_consumed   = 0;
  for (std::size_t offset = 0; offset < body.size(); offset += chunk_size) {
    std::size_t this_chunk = std::min(chunk_size, body.size() - offset);
    std::size_t consumed   = reader.put(boost::asio::buffer(body.data() + offset, this_chunk), ec);
    BOOST_TEST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(consumed, this_chunk);
    total_consumed += consumed;
  }

  reader.finish(ec);
  BOOST_TEST_REQUIRE(!ec);

  BOOST_CHECK_EQUAL(body_value.parts_.size(), 1u);
  if (body_value.parts_.size() >= 1) {
    BOOST_CHECK_EQUAL(body_value.parts_[0].name, "chunked");
    BOOST_CHECK(std::holds_alternative<std::string>(body_value.parts_[0].body_));
    if (std::holds_alternative<std::string>(body_value.parts_[0].body_)) {
      BOOST_CHECK_EQUAL(std::get<std::string>(body_value.parts_[0].body_), json_data);
    }
  }
}

/// 测试 8: value_type::to_json() 输出
/// 验证解析结果能正确转换为 JSON
BOOST_AUTO_TEST_CASE(value_type_to_json) {
  const std::string boundary = "----JsonOutputBoundary";
  std::string body;
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"field1\"\r\n";
  body += "Content-Type: application/json\r\n";
  body += "\r\n";
  body += R"({"a":1})";
  body += "\r\n";
  body += "--" + boundary + "--\r\n";

  boost::beast::http::request<boost::beast::http::empty_body> req;
  req.set(boost::beast::http::field::content_type, "multipart/form-data; boundary=" + boundary);

  doodle::http::multipart_body::value_type body_value;
  doodle::http::multipart_body::reader reader{req, body_value};

  boost::system::error_code ec;
  reader.init(boost::optional<std::uint64_t>(body.size()), ec);
  BOOST_TEST_REQUIRE(!ec);
  reader.put(boost::asio::buffer(body), ec);
  BOOST_TEST_REQUIRE(!ec);
  reader.finish(ec);
  BOOST_TEST_REQUIRE(!ec);

  nlohmann::json j = body_value.to_json();
  BOOST_TEST_MESSAGE("to_json() output: " << j.dump());
  BOOST_CHECK(j.contains("field1"));
  BOOST_CHECK_EQUAL(j["field1"]["a"].get<int>(), 1);
}

BOOST_AUTO_TEST_SUITE_END()