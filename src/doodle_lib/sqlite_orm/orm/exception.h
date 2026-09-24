#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/beast/http/status.hpp>

namespace doodle::orm {
/// ORM 异常，自带一个 HTTP 状态码。
///
/// **分层说明**：这一层本来不该知道 HTTP。但 sqlite_orm_exception 事实上只有 HTTP 层
/// 一个消费者（http_session_data.cpp 把库错误映射成响应状态），而不同类型的库错误
/// 必须映射成不同的状态码 —— 「查不到行」是 404，「约束冲突」是 400。
/// 与其让 HTTP 层去猜异常文本，不如由**抛出点**自己声明语义。
///
/// 默认 bad_request，保持既有行为不变：原有的抛出点都是约束类错误
/// （外键 NO ACTION 撞 FOREIGN KEY constraint failed、rebind_range 参数数量不匹配等），
/// 属于**调用方的输入问题**而不是服务端故障。
///
/// 访问器名沿用 doodle_error 的 code_status_()，让 HTTP 层两处 catch 写法一致。
class sqlite_orm_exception : public std::runtime_error {
 public:
  explicit sqlite_orm_exception(
      const std::string& message, boost::beast::http::status in_status = boost::beast::http::status::bad_request
  )
      : std::runtime_error(message), status_(in_status) {}

  boost::beast::http::status code_status_() const { return status_; }

 private:
  boost::beast::http::status status_{boost::beast::http::status::bad_request};
};

// 重新bing range参数时, 参数数量必须与set_range时一致, 否则抛出异常
struct rebind_range_size_mismatch_exception : public sqlite_orm_exception {
  rebind_range_size_mismatch_exception() : sqlite_orm_exception("rebind_range中的值数量必须与set_range时一致") {}
};
}  // namespace doodle::orm
