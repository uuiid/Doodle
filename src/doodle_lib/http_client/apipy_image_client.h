//
// Created by TD on 2025.
//

#pragma once

#include <doodle_core/metadata/seedance2/image_request.h>
#include <doodle_core/metadata/seedance2/task.h>

#include "ai_client_base.h"
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::http::seedance2 {

/**
 * API易(apiyi.com) 图片客户端。
 *
 * 与另外两个子类**根本不同**: 这个后端的图片接口是**同步单次调用** ——
 * 一次 HTTP 请求全程阻塞, 结果直接在响应里返回。
 *   * 没有 task_id, 没有查询接口, 没有取消接口;
 *   * 客户端断开后上游仍会跑完并**照常计费**(官方最佳实践页明确写了这一点)。
 * 所以 timeout 是本后端的第一原则, 见 g_request_timeout。
 *
 * 于是基类的流程纯虚在这里只能是:
 *   * submit_task  —— 真正干活的那个: 一次调用直接拿终态 + 图片文件;
 *   * query_task   —— 没有对应接口, 永远不该被调用(见 .cpp 的说明);
 *   * cancel_task  —— 同上;
 *   * download_result —— 下载动作发生在 submit_task 内部, 所以也没有调用者。
 *
 * 请求形状(data_request_)与中转站**共用** 一份类型: seedance2::transfer_station_request_t
 * (见 doodle_core/metadata/seedance2/image_request.h)。不再原地裸读 JSON 键 —— 键名与
 * 取值域由那个结构体的 to_json/from_json 保证, 本类只在该结构体上做属己的定稿:
 *   1) 把出图的标量从在中转站叫法(model/prompt/quality/aspectRatio)落成 API易 叫法
 *      (同值改键: aspectRatio -> size) —— 见 to_apipy_json;
 *   2) 取出参考图 URL 列表(那条路由本类自己定形状, 见 extract_reference_image_urls);
 *   3) 校验参考图 URL 是本站图片路由(checked_reference_url), 再读成本地字节。
 *
 * 为什么能共用: 两家的键集本质一样(都是 OpenAI 兼容的 model/prompt/参考图), 差别只在
 * 一处**键名同义**: 中转站把它叫 aspectRatio, API易 的 generations 叫 size。数据模型
 * 多一份就要多维护一份键名映射, 且必然与真实契约一起漂移; 于是这里选择复用, 只把这一个
 * 差异写在本文件的 to_apipy_json 里。
 *
 * ─── 参考图 URL 的契约: 只有本站图片路由 ─────────────────────────────────────
 * 与另两个后端**相反**, 本后端**不**把参考图 URL 改写成公网签名 URL(不调 to_public_url,
 * 也不再取公网 IP): 参考图是**我们自己去读的**, 而不是交给第三方去取的 —— 上游拿到的是
 * 图片字节(文件负担), 从头到尾看不到 URL。所以那条 URL 只需要「本进程读得到」。
 * 于是它被收窄成一条可枚举的形状: /api/seedance2/pictures/{uuid}{ext}
 * (reg.h 的 seedance2_pictures), 由 checked_reference_url 强制; 其余一律 400。
 *
 * ─── 文生图 / 改图两条路 ─────────────────────────────────────────────────────
 * 按结构体里**有没有参考图**(reference_images_: 由 content[].image_url 或
 * input[].content[].image_url 抽出)分流:
 *   * 没有参考图 -> POST /v1/images/generations, JSON 体(原来的那条路);
 *   * 有参考图   -> POST /v1/images/edits,       multipart/form-data。
 * 两条路都显式要 response_format=url, 结果处理完全一致(见 submit_task)。
 */
class DOODLELIB_API apipy_image_client final : public ai_client_base {
  constexpr static std::string_view g_default_host{"https://api.apiyi.com"};

 public:
  /// 本后端是同步长请求, 一次调用阻塞到出图。
  /// 官方最佳实践给的推荐值是 GPT-Image-2.5 系列 360 秒(GPT-Image-2-vip 是 300 秒),
  /// 这里取 360 起步。为什么必须留足: 客户端主动断开后上游仍会跑完并**照常计费**
  /// (官方原话), 所以 timeout 设小了不是"快速失败", 而是"花了钱拿不到图"。
  /// 注意这不是本链路唯一的上限 —— http_client_core.h 的 read_and_write 内部还有一次
  /// 硬编码的 expires_after(900s) 覆盖「写请求 + 读响应头」, 两者取小, 因此真正的天花板是
  /// 900 秒。将来本值若调到 900 以上, 必须同步改那一处。
  ///
  /// 放在这里而不是 doodle_core/configure/static_value.h: 那个头是全工程范围的,
  /// 动它会触发全量重编(实测 469 个编译单元), 而这两个常量只有本文件用。
  constexpr static std::chrono::seconds g_request_timeout{360};
  /// 默认模型; 结构体里的 model_ 非空就用它的
  constexpr static std::string_view g_default_model{"gpt-image-2.5-vip"};

  /// 图片编辑接口。与生成接口的差别不只是路径: 它是 multipart/form-data, 参考图必须**作为
  /// 文件负担**上传(见 build_edit_multipart)。前端给的素材 URL 是本机可达的站点内地址,
  /// 我们要先把它读成本地字节再编进去 —— 也就是下游拿不到 URL, 只能拿字节。
  constexpr static std::string_view g_edits_path{"/v1/images/edits"};

  /// edits 里每个参考图负担的字段名。官方要求重复传**同名** image 字段(数组语义),
  /// 顺序即 prompt 里「图1/图2/图3」的指代顺序, 所以这里必须是常量。
  constexpr static std::string_view g_edit_image_field{"image"};

  /// 参考图张数上限。官方没有硬上限(只说可重复传), 但每张推荐 ≤10MB, 而 multipart 请求体
  /// 是在内存里整体拼出来的, 所以这里按"最坏情况 10MB × 8 = 80MB"画一条线, 超过就早失败,
  /// 而不是等上游网关先掐断。张数顺序有意义, 所以只限数量, 不做去重。
  constexpr static std::size_t g_max_reference_images{8};

  /// edits 的请求体(整份 multipart/form-data 字节流 + 它的 boundary)。
  /// 单独成结构体而不是两个返回值: boundary 必须同时出现在 Content-Type 头和体里,
  /// 绑在一起才不会出现"头里一个 boundary、体里另一个"的错配。
  /// content_type() 就地内联: 嵌套结构体的成员不会由 DOODLELIB_API 导出(那个宏只作用于
  /// 外层类), 测试只链公开头时会在链接期找不到符号。
  struct edits_form_t {
    std::string boundary_;
    std::string body_;

    /// Content-Type 头的值: `multipart/form-data; boundary=...`
    [[nodiscard]] std::string content_type() const { return "multipart/form-data; boundary=" + boundary_; }
  };

  /// 一张参考图: 原始 URL(写进日志/报错) + 它的字节内容 + 文件名 + MIME
  struct edit_reference_t {
    std::string url_;
    std::string filename_;
    std::string mime_;
    std::string bytes_;
  };

  explicit apipy_image_client(boost::asio::ssl::context& in_ctx)
      : ai_client_base(std::string{g_default_host}, in_ctx) {}

  /// 一次调用即终态: 发请求 → 解析 → **当场把图片下载落盘** → 填 result_files_ /
  /// result_file_paths_ 返回。本后端没有 query_task 可以之后再取, 也没必要重写 run_task
  /// (基类默认就是转发到这里)。
  boost::asio::awaitable<run_task_result_t> submit_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<run_task_result_t> query_task(const doodle::seedance2::task& in_task) override;
  boost::asio::awaitable<void> cancel_task(const std::string& in_task_id) override;
  boost::asio::awaitable<void> download_result(run_task_result_t& in_data) override;

  request_info_t collect_request_info(const nlohmann::json& in_request) const override;
  std::size_t default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const override;

  // ─── 纯函数, 便于不联网单测 ────────────────────────────────────────────────

  /// 从响应体里取出图片 URL 列表。
  /// 兼容三种形状: data[0].url(R2 链接) / data[0].b64_json(历史版本含 data: 前缀) /
  /// 以及上游报错时的 error 对象(返回空)。
  static std::vector<std::string> extract_image_urls(const nlohmann::json& in_body);

  /// 取 data[0].b64_json 并剥掉可能存在的 data: 前缀(2026-07 实测为纯 base64,
  /// 但历史版本曾带前缀, 官方也要求做这个检测)。没有该字段时返回空串。
  static std::string extract_b64_payload(const nlohmann::json& in_body);

  /// 按 HTTP 状态码与响应体判定任务状态。
  /// 2xx 且有图 → succeeded; 其余 → failed(附错误码)。
  static doodle::seedance2::task_status parse_status(const nlohmann::json& in_body, int in_status_code);

  /// 取上游错误码(error.code / error.type); 没有则空串
  static std::string get_error_code(const nlohmann::json& in_body);

  /// 把 API易 的原生请求体(前端 PUT 进来的形状)翻译成结构体读得懂的样子:
  /// size -> aspectRatio, content[] -> images。纯变换, 不校验取值。
  /// parse_request 用它, 单测也直接钉它。
  [[nodiscard]] static nlohmann::json normalize_apipy_request(const nlohmann::json& in_request);

  // ─── 纯函数, 便于不联网单测(改图这条路) ────────────────────────────────────

  /// 把参考图 URL 列表编成一份 multipart/form-data 请求体。
  ///
  /// 文本字段由调用方按 API易 叫法给全(见 to_apipy_json), 本函数**只做编码**:
  /// 每个 reference 编成一个同名 g_edit_image_field 的部分(重复字段名 = 数组语义),
  /// 顺序原样保持 —— 顺序即 prompt 里「图1/图2/图3」的指代顺序。
  ///
  /// 纯函数: 不读盘、不联网, 输入什么就编什么。
  static edits_form_t build_edit_multipart(
      const nlohmann::json& in_apipy_body, const std::vector<edit_reference_t>& in_references
  );

  /// 站点内图片 URL 拆出来的两个标识: 素材 uuid 与扩展名
  struct site_image_id_t {
    uuid id_{};
    std::string extension_{};
  };

  /// 把站点内图片 URL 拆成 {uuid, 扩展名}; 不是本站图片(绝对 URL、data: 等)时返回 nullopt。
  /// 只认 /api/seedance2/pictures/{uuid}{ext} 这一条形状 —— 它是本项目唯一把图片交给第三方
  /// 的出口(见 http_method/seedance2/reg.h 的 seedance2_pictures)。带查询串(签名参数)也能认。
  /// 纯函数(不碰 g_ctx), 因此可以在没有 app_base 的单测里直接钉。
  static FSys::path parse_site_image_id(std::string_view in_url);

  /// 由扩展名推断 MIME; 未知按 image/png(官方只接受 png/jpg/webp)
  static std::string mime_of_image(const std::string& in_ext);

 private:
  /// base64 结果就地落盘(core_set 缓存目录), 返回文件路径。
  /// b64_json 模式(未切 image2_OSS 分组时会发生)走这条; url 模式走基类 download_raw_file。
  FSys::path write_b64_to_cache(std::string_view in_b64, std::string_view in_ext) const;

  apipy_image_client::edit_reference_t load_reference_image(std::string_view in_url);
};

}  // namespace doodle::http::seedance2
