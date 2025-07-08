//
// Created by TD on 25-5-8.
//

#pragma once
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http::model_library {
DOODLE_HTTP_FUN(context, get, "api/doodle/model_library/context", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(assets, get, "api/doodle/model_library/assets", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(assets, post, "api/doodle/model_library/assets", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(assets, put, "api/doodle/model_library/assets/{id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(assets, delete_, "api/doodle/model_library/assets/{id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(assets_tree, get, "api/doodle/model_library/assets_tree", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(assets_tree, post, "api/doodle/model_library/assets_tree", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(assets_tree, put, "api/doodle/model_library/assets_tree/{id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(assets_tree, delete_, "api/doodle/model_library/assets_tree/{id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

class pictures_base : public http_jwt_fun {
 protected:
  void create_thumbnail_mp4(const FSys::path& in_data_path, const FSys::path& in_path, FSys::path in_name);
  void create_thumbnail_gif(const FSys::path& in_data_path, const FSys::path& in_path, FSys::path in_name);
  void create_thumbnail_image(const std::string& in_data, const FSys::path& in_path, FSys::path in_name);
  boost::asio::awaitable<boost::beast::http::message_generator> thumbnail_get(session_data_ptr in_handle);
  boost::asio::awaitable<boost::beast::http::message_generator> thumbnail_post(session_data_ptr in_handle);
  boost::asio::awaitable<boost::beast::http::message_generator> thumbnail_404(session_data_ptr in_handle);
  std::shared_ptr<FSys::path> root_;

 public:
  using http_jwt_fun::http_jwt_fun;
  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
};
// "api/doodle/pictures/{id}"
DOODLE_HTTP_FUN_CONST(
    pictures, post,
    ucom_t{}.ro<capture_id_t>() / "api" / "doodle" / "pictures" / make_cap(g_uuid_regex, &capture_id_t::id_),
    model_library::pictures_base, const FSys::path in_root
) {
  root_ = std::make_shared<FSys::path>(in_root);
  if (!FSys::exists(*root_)) FSys::create_directories(*root_);
}
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN_CONST(
    pictures, get,
    ucom_t{}.ro<capture_id_t>() / "api" / "doodle" / "pictures" / make_cap(g_uuid_regex, &capture_id_t::id_),
    model_library::pictures_base, const FSys::path in_root
) {
  root_ = std::make_shared<FSys::path>(in_root / "previews");
  if (!FSys::exists(*root_)) FSys::create_directories(*root_);
}
DOODLE_HTTP_FUN_END()
// "api/doodle/pictures/thumbnails/{id}"
DOODLE_HTTP_FUN_CONST(
    pictures_thumbnails, get,
    ucom_t{}.ro<capture_id_t>() / "api" / "doodle" / "pictures" / "thumbnails" /
        make_cap(g_uuid_regex, &capture_id_t::id_),
    model_library::pictures_base, const FSys::path in_root
) {
  root_ = std::make_shared<FSys::path>(in_root / "thumbnails");
  if (!FSys::exists(*root_)) FSys::create_directories(*root_);
}
DOODLE_HTTP_FUN_END()
// "api/doodle/model_library/assets_tree/{id}/assets/{assets_id}"
struct capture_assets_id_t {
  std::string id_;
  std::string assets_id_;
};
DOODLE_HTTP_FUN(
    assets_tree_link, post,
    ucom_t{}.ro<capture_assets_id_t>() / "api" / "doodle" / "model_library" / "assets_tree" /
        make_cap(g_uuid_regex, &capture_assets_id_t::id_) / "assets" /
        make_cap(g_uuid_regex, &capture_assets_id_t::assets_id_),
    http_jwt_fun
)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(
    assets_tree_link, delete_,
    ucom_t{}.ro<capture_assets_id_t>() / "api" / "doodle" / "model_library" / "assets_tree" /
        make_cap(g_uuid_regex, &capture_assets_id_t::id_) / "assets" /
        make_cap(g_uuid_regex, &capture_assets_id_t::assets_id_),
    http_jwt_fun
)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http::model_library
