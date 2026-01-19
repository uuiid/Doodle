#pragma once
#include <doodle_core/core/global_function.h>
#include <doodle_core/core/http_client_core.h>
#include <doodle_core/metadata/studio.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

#include <memory>

// #include <asio/experimental/as_single.hpp>

namespace doodle::dingding {
class client : public std::enable_shared_from_this<client> {
  using https_client_core     = doodle::http::http_client_ssl;
  using https_client_core_ptr = std::shared_ptr<https_client_core>;

  using timer_t = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>::as_default_on_t<boost::asio::steady_timer>;
  using timer_ptr_t = std::shared_ptr<timer_t>;

  https_client_core_ptr http_client_core_ptr_{};      // 新版本api
  https_client_core_ptr http_client_core_ptr_old_{};  // 旧版本api
  std::string access_token_;

  std::string app_key;
  std::string app_secret;
  chrono::sys_time_pos token_time_;

  bool auto_expire_;

  boost::asio::awaitable<void> refresh_token();
  bool token_is_valid();

  template <typename Req>
  std::decay_t<Req> header_operator_req(Req&& in_req) {
    in_req.set(boost::beast::http::field::accept, "application/json");
    in_req.set(boost::beast::http::field::content_type, "application/json");
    in_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    in_req.keep_alive(true);
    in_req.prepare_payload();
    return std::move(in_req);
  }

  template <typename Resp>
  void header_operator_resp(Resp& in_resp) {}

 public:
  explicit client(boost::asio::ssl::context& in_ctx)
      : http_client_core_ptr_(std::make_shared<https_client_core>("https://api.dingtalk.com/", in_ctx)),
        http_client_core_ptr_old_(std::make_shared<https_client_core>("https://oapi.dingtalk.com/", in_ctx)) {};
  ~client() = default;

  // 初始化, 必须调用, 否则无法使用, 获取授权后将自动2小时刷新一次
  void access_token(const std::string& in_app_key, const std::string& in_app_secret);

  boost::asio::awaitable<std::string> get_user_by_mobile(const std::string& in_mobile);

  struct attendance_update {
    chrono::local_time_pos begin_time_;
    chrono::local_time_pos end_time_;
    std::int32_t biz_type_;
    std::string tag_name_;
    std::string sub_type_;
    std::string prcoInst_id_;
    // form json
    friend void from_json(const nlohmann::json& j, attendance_update& p) {
      std::istringstream l_time_stream{j.at("begin_time").get<std::string>()};
      l_time_stream >> chrono::parse("%F %T", p.begin_time_);

      l_time_stream = std::istringstream{j.at("end_time").get<std::string>()};
      l_time_stream >> chrono::parse("%F %T", p.end_time_);

      p.biz_type_    = j.at("biz_type").get<std::int32_t>();
      p.tag_name_    = j.at("tag_name").get<std::string>();
      p.sub_type_    = j.at("sub_type").get<std::string>();
      p.prcoInst_id_ = j.at("procInst_id").get<std::string>();
    }
  };

  // 获取考勤数据
  boost::asio::awaitable<std::vector<attendance_update>> get_attendance_updatedata(
      const std::string& in_user_id, const chrono::local_time_pos& in_work_date
  );
};

using client_ptr = std::shared_ptr<client>;

class dingding_company {
  std::map<uuid, client_ptr> client_map_;
  boost::asio::strand<boost::asio::io_context::executor_type> executor_{};

 public:
  explicit dingding_company() : executor_(boost::asio::make_strand(doodle::g_io_context())) {}

  ~dingding_company() = default;
  std::shared_ptr<boost::asio::ssl::context> ctx_ptr;

  boost::asio::awaitable<client_ptr> make_client(const studio& in_studio) const;

 private:
};
}  // namespace doodle::dingding