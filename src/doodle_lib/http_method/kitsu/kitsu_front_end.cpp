//
// Created by TD on 24-9-26.
//

#include "kitsu_front_end.h"

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/zlib_deflate_file_body.h>
#include <doodle_lib/http_method/kitsu.h>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
namespace doodle::http {

std::tuple<bool, std::shared_ptr<http_function>> kitsu_front_end_url_route_component::set_match_url(
    boost::urls::segments_ref in_segments_ref, const std::shared_ptr<http_function>& in_data
) const {
  if (!in_segments_ref.empty() && in_segments_ref.front() == "api") return {false, {}};
  return {true, in_data};
}

namespace {
FSys::path make_doc_path(const std::shared_ptr<FSys::path>& in_root, const boost::urls::segments_ref& in_) {
  auto l_path = *in_root;
  for (auto&& i : in_) {
    l_path /= i;
  }
  if (in_.size() == 0) {
    l_path /= "index.html";
  }
  if (!in_.empty() && in_.front() != "api" && !FSys::exists(l_path)) {
    l_path = *in_root / "index.html";
  }
  return l_path;
}

std::string get_file_deflate(const FSys::path& in_path) {
  boost::iostreams::file_source l_source{in_path.generic_string()};
  // boost::iostreams::filtering_streambuf<boost::iostreams::input> l_stream{};
  boost::iostreams::filtering_istream l_filter{};
  l_filter.push(boost::iostreams::zlib_compressor{});
  l_filter.push(l_source);
  std::string l_str{};
  boost::iostreams::stream<boost::iostreams::back_insert_device<std::string>> l_str_stream{l_str};
  boost::iostreams::copy(l_filter, l_str_stream);
  return l_str;
}
}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> kitsu_front_end::get(session_data_ptr in_handle) {
  auto l_path = make_doc_path(root_path_, in_handle->url_.segments());

  if (l_path.filename() == "index.html") {
    co_return in_handle->make_msg(l_path, kitsu::mime_type(l_path.extension()));
  }
  static std::set<FSys::path> l_binary_exts{".png", ".jpg", ".jpeg", ".gif",  ".ico", ".svgz",
                                            ".map", ".exe", ".mp4",  ".webm", ".zip"};
  if (l_binary_exts.contains(l_path.extension()))
    co_return in_handle->make_msg(l_path, kitsu::mime_type(l_path.extension()));

  std::string l_value{};
  if (!cache_->Cached(l_path)) cache_->Put(l_path, get_file_deflate(l_path));
  l_value = *cache_->Get(l_path);

  co_return in_handle->make_msg(
      std::move(l_value), http_header_ctrl{.mine_type_ = kitsu::mime_type(l_path.extension()), .is_deflate_ = true}
  );
}
boost::asio::awaitable<boost::beast::http::message_generator> kitsu_front_end::head(session_data_ptr in_handle) {
  auto l_path = make_doc_path(root_path_, in_handle->url_.segments());
  boost::beast::http::response<boost::beast::http::file_body> l_res{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_res.set(boost::beast::http::field::content_type, kitsu::mime_type(l_path.extension()));
  l_res.content_length(FSys::file_size(l_path));
  l_res.keep_alive(in_handle->keep_alive_);
  l_res.prepare_payload();
  co_return std::move(l_res);
}

}  // namespace doodle::http