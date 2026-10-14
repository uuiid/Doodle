//
// Created by TD on 2024/2/29.
//

#pragma once

#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/core/http_client_core.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

namespace doodle::http::seedance2 {
class DOODLELIB_API ai_client_base : public std::enable_shared_from_this<ai_client_base> {
  using http_client_t     = doodle::http::http_client_ssl;
  using http_client_ptr_t = std::shared_ptr<http_client_t>;

 protected:
  http_client_ptr_t http_client_ptr_{};
  std::string token_;
  logger_ptr logger_{spdlog::default_logger()};

 public:
  explicit ai_client_base(std::string in_host_url, boost::asio::ssl::context& in_ctx)
      : http_client_ptr_{std::make_shared<http_client_t>(std::move(in_host_url), in_ctx)} {}

  /// 注意 in_logger 可能为空：`g_logger_ctrl().get_http()` 就是 `spdlog::get("http")`，
  /// 在 http logger 还没建起来的宿主里（单测、只初始化了一半的 worker）返回 nullptr。
  /// 直接存下来会在第一次 `logger_->xxx()` 时崩在 0x8 上 —— 所以这里回落到 default_logger。
  void set_logger(logger_ptr in_logger) { logger_ = in_logger ? std::move(in_logger) : spdlog::default_logger(); }
  void set_token(const std::string& in_token) { token_ = in_token; }
  const std::string& get_token() const { return token_; }

  /// 签名公网 URL 用的密钥（kitsu_ctx_t::secret_，仅主服务器可用）。
  /// 为空表示不签名 —— 本地/worker 构建拿不到注册表密钥时走这条路径。
  void set_url_sign_secret(const std::string& in_secret) { url_sign_secret_ = in_secret; }
  const std::string& get_url_sign_secret() const { return url_sign_secret_; }

  /**
   * 一次任务周期(提交 / 查询 / 下载)的统一结果。
   *
   * 提交与查询原本各有一个结构体, 但调用方(seedance2_task_run_manager)对两者做的是**同一件事**:
   * 拿状态 → 落库 → 广播。合并成一个结构体后, run_task 的返回值不再分裂, 落库代码只有一份。
   * 各字段的"哪条路径会填"见下方注释 —— 空值不代表出错, 只代表这次走的是另一条路径。
   */
  struct run_task_result_t {
    /// 提交路径: 外部服务的任务 id; 查询路径: 原样带回 in_task.task_id_
    std::string task_id_;
    doodle::seedance2::task_status status_;
    bool is_timeout_{false};
    nlohmann::json data_response_;

    /// 推理任务（task_type::inference）的输出文本；生成类任务为空
    std::string result_text_{};
    /// 推理任务按 text.format 的 json_schema 解析出来的结果；未要求结构化输出时为 null
    nlohmann::json result_json_{};
    /// 结果文件。三条路径都可能填:
    ///   * 同步单次调用后端(API易 图片): submit_task 内当场下载落盘;
    ///   * 异步后端(seedance2 / 中转站): 下载后填(由 run_task 驱动器在查询成功后调用 download_result);
    ///   * 推理任务: 结果是一段文本, 两个字段都为空。
    std::vector<std::string> result_files_;
    std::vector<FSys::path> result_file_paths_;
    /// token 用量。推理任务 = 方舟响应的 usage; 生成类任务 = 本次实际结算的消耗(查询路径给出,
    /// 提交路径为 0 —— 提交时还不知道花了多少)。
    std::int64_t input_tokens_{0};
    std::int64_t output_tokens_{0};
    std::int64_t total_tokens_{0};
    /// 生成类任务在**查询路径**结算出的实际消耗(点数/代币), 提交路径为 0。
    /// 与 total_tokens_ 的区别: total_tokens_ 是大模型自报的用量(推理口径), 这个是本站的计费口径。
    std::int64_t completion_tokens_{0};
    /// 外部服务返回的错误码（方舟的 error.code），用来区分内容违规 / 限流 / 配置错误
    std::string error_code_{};
  };

  // 从请求体中提取的公共字段, 各后端请求格式不同, 由子类按自己的格式解析
  struct request_info_t {
    std::string model_{};        // 模型名称
    std::string resolution_{};   // 分辨率
    std::string text_prompt_{};  // 提示词文本
  };

  /**
   * 提交任务(异步后端的入口)。
   *
   * 「提交」的契约: 把任务交给外部服务, **不等结果**。
   * - 异步生成类(picture/video): 返回 queued + task_id_, 之后靠 query_task 轮询;
   * - 同步单次调用类(方舟 Responses、API易 图片): 一次调用就带回终态, 直接返回
   *   succeeded/failed, 结果(文本 / 文件)也一并填好。
   *
   * 参数从「纯 JSON 体」换成整个 task, 因为需要区分任务类型; 另外 in_task.user_id_ 会作为
   * 签名 URL 里的人员(uid), 让第三方取图时仍带上授权身份。
   */
  virtual boost::asio::awaitable<run_task_result_t> submit_task(const doodle::seedance2::task& in_task) = 0;

  /**
   * 执行一次任务周期 —— **调用方唯一需要认的名字**。
   *
   * 按 in_task.status_ 分派, 每次调用只做一件事(不做等待/轮询; 轮询由调用方的定时器负责):
   * - `preparing`: 提交。co_await submit_task() 后**直接返回**, 无论它是 queued 还是终态;
   * - 其它(queued/running): 查询。co_await query_task(); 若查得 succeeded, **顺手把结果下载落盘**
   *   (download_result), 于是返回给调用方的 result_file_paths_ 已经是本地文件; running/queued
   *   等中间态原样返回, 调用方等下一次轮询。
   *
   * 为什么把这段放在基类: 「先提交、再查询、成功后下载」是与**后端无关**的流程, 三个子类的
   * 差别只在那三个原语里。放在这里, 调用方(seedance2_task_run_manager)就只剩落库与广播。
   *
   * 这是虚函数(不是纯虚): 子类默认不需要重写, 因为流程是统一的。
   */
  virtual boost::asio::awaitable<run_task_result_t> run_task(const doodle::seedance2::task& in_task) {
    if (in_task.status_ == doodle::seedance2::task_status::preparing) co_return co_await submit_task(in_task);

    auto l_result = co_await query_task(in_task);
    l_result.task_id_ = in_task.task_id_;
    if (l_result.status_ == doodle::seedance2::task_status::succeeded) co_await download_result(l_result);
    co_return l_result;
  }

  virtual boost::asio::awaitable<void> cancel_task(const std::string& in_task_id)                        = 0;

  /// 查询一次任务状态。只在 run_task 的查询路径被调用, 见上面的说明。
  virtual boost::asio::awaitable<run_task_result_t> query_task(const doodle::seedance2::task& in_task)   = 0;

  /// 下载结果文件并填进 in_data.result_file_paths_。
  ///
  /// 按值传入、就地修改后由 run_task 把同一个对象交给调用方 —— 这样既不用 out 参数
  /// (本库禁止用非常量引用/指针出参返回结果), 也不用为了一个下载动作复制两份结果。
  virtual boost::asio::awaitable<void> download_result(run_task_result_t& in_data)                       = 0;

  // 解析请求体, 提取模型 / 分辨率 / 提示词
  virtual request_info_t collect_request_info(const nlohmann::json& in_request) const                    = 0;
  // 默认消耗的 tokens 数
  virtual std::size_t default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const    = 0;

  // ─── 公网素材 URL ──────────────────────────────────────────────────────────
  // 请求体里的素材路径是「站点内相对路径」(如 /api/seedance2/pictures/xxx.png), 第三方取不到,
  // 需要补公网前缀; 而且这些路由现在要鉴权, 所以同时追加短时效签名(url_sign_t)。
  // 绝对 URL(对象存储签名 URL、方舟自己的 URL)一律原样保留。

  /// 单个 URL 的改写: 站点内路径 -> http://{ip}:{core_set::public_url_port_}{path}?exp=..&uid=..&sig=..
  /// 带 scheme 的 URL(http/https/data/blob 等)一律原样返回;
  /// in_secret 为空时只补前缀、不追加签名。
  static std::string to_public_url(
      std::string_view in_url, std::string_view in_ip, std::string_view in_secret = {}, std::string_view in_uid = {}
  );

  /// 遍历请求体里所有素材 URL 字段并对每个做改写, 覆盖两种请求形状:
  ///   * 生成 API:  content[].{image_url,video_url,audio_url}.url   (对象)
  ///   * Responses: input[].content[].image_url                     (字符串)
  static nlohmann::json add_ip_to_req(
      const nlohmann::json& in_req, std::string_view in_ip, std::string_view in_secret = {},
      std::string_view in_uid = {}
  );

  /// 遍历一个 content 数组里的素材 URL 字段(callback 收到可写的 URL 字符串)
  template <typename Fn>
  static void for_each_media_url_in_content(nlohmann::json& in_content, Fn&& in_fn) {
    if (!in_content.is_array()) return;
    for (auto& l_item : in_content) {
      if (!l_item.is_object()) continue;
      for (const auto* l_key : {"image_url", "video_url", "audio_url"}) {
        if (!l_item.contains(l_key)) continue;
        auto& l_holder = l_item.at(l_key);
        // 生成 API: {"image_url": {"url": "..."}}
        if (l_holder.is_object() && l_holder.contains("url") && l_holder.at("url").is_string())
          in_fn(l_holder.at("url"));
        // Responses: {"type":"input_image", "image_url": "..."} —— image_url 是字符串
        else if (l_holder.is_string())
          in_fn(l_holder);
      }
    }
  }

  /// 遍历请求体里所有素材 URL 字段(callback 收到可写的 URL 字符串), 两种形状都覆盖
  template <typename Fn>
  static void for_each_media_url(nlohmann::json& in_req, Fn&& in_fn) {
    if (!in_req.is_object()) return;
    if (in_req.contains("content")) for_each_media_url_in_content(in_req.at("content"), in_fn);
    if (in_req.contains("input") && in_req.at("input").is_array())
      for (auto& l_input : in_req.at("input"))
        if (l_input.is_object() && l_input.contains("content"))
          for_each_media_url_in_content(l_input.at("content"), in_fn);
  }

 protected:
  boost::asio::awaitable<std::string> get_ip_str();
  boost::asio::awaitable<FSys::path> download_raw_file(std::string_view in_file_url);

  std::string url_sign_secret_{};
};
}  // namespace doodle::http::seedance2