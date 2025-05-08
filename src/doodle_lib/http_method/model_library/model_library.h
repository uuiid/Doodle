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

DOODLE_HTTP_FUN(assets_modify, post, "api/doodle/model_library/assets/{id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(assets, delete_, "api/doodle/model_library/assets/{id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(assets, patch, "api/doodle/model_library/assets", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(assets_tree, get, "api/doodle/model_library/assets_tree", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(assets_tree, post, "api/doodle/model_library/assets_tree", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(assets_tree, patch, "api/doodle/model_library/assets_tree", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(assets_tree_modify, post, "api/doodle/model_library/assets_tree/{id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(assets_tree, delete_, "api/doodle/model_library/assets_tree/{id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

class pictures_base : public http_jwt_fun {
 protected:
  void create_thumbnail_mp4(const FSys::path& in_data_path, const FSys::path& in_path, const std::string& in_name);
  void create_thumbnail_gif(const FSys::path& in_data_path, const FSys::path& in_path, const std::string& in_name);
  void create_thumbnail_image(const std::string& in_data, const FSys::path& in_path, const std::string& in_name);
  boost::asio::awaitable<boost::beast::http::message_generator> thumbnail_get(session_data_ptr in_handle);
  boost::asio::awaitable<boost::beast::http::message_generator> thumbnail_post(session_data_ptr in_handle);
  boost::asio::awaitable<boost::beast::http::message_generator> thumbnail_404(session_data_ptr in_handle);
  std::shared_ptr<FSys::path> root_;

 public:
  using http_jwt_fun::http_jwt_fun;
  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
};
DOODLE_HTTP_FUN_CONST(
    pictures, post, "api/doodle/pictures/{id}", model_library::pictures_base, const FSys::path in_root
) {
  root_ = std::make_shared<FSys::path>(in_root);
  if (!FSys::exists(*root_)) FSys::create_directories(*root_);
}
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN_CONST(
    pictures, get, "api/doodle/pictures/{id}", model_library::pictures_base, const FSys::path in_root
) {
  root_ = std::make_shared<FSys::path>(in_root / "previews");
  if (!FSys::exists(*root_)) FSys::create_directories(*root_);
}
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN_CONST(
    pictures_thumbnails, get, "api/doodle/pictures/thumbnails/{id}", model_library::pictures_base,
    const FSys::path in_root
) {
  root_ = std::make_shared<FSys::path>(in_root / "thumbnails");
  if (!FSys::exists(*root_)) FSys::create_directories(*root_);
}
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(label, get, "api/doodle/model_library/label", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(label, post, "api/doodle/model_library/label", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(label, put, "api/doodle/model_library/label/{id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(label, delete_, "api/doodle/model_library/label", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(label_link, post, "api/doodle/model_library/label/{id}/assets/{assets_id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(label_link, delete_, "api/doodle/model_library/label/{id}/assets/{assets_id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http::model_library
