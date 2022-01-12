//
// Created by TD on 2022/1/11.
//
#include <doodle_lib/doodle_lib_fwd.h>
//#include <doodle_lib/metadata/metadata.h>
namespace doodle {

enum class metadata_type;

namespace database_n {
class DOODLELIB_API filter {
 public:
  using time_point = chrono::sys_time_pos;

 private:
 public:
  filter();

  std::optional<std::int64_t> _id;
  std::optional<std::int64_t> _parent_id;
  std::optional<metadata_type> _meta_type;
  std::optional<time_point> _begin;
  std::optional<time_point> _end;
  std::optional<std::string> _uuid_path;
  std::optional<std::int64_t> _episodes;
  std::optional<std::int64_t> _shot;
  std::optional<string> _assets;

  std::uint64_t _beg_off_id;
  /**
   * @brief 提取数据的条数，默认为 1000
   */
  std::uint16_t _off_size;
};
}  // namespace database_n

class DOODLELIB_API database_task : public process_t<database_task> {
 private:
  class impl;
  std::unique_ptr<impl> p_i;
  void select_db();

 public:
  using base_type = process_t<database_task>;
  /**
   * @brief 使用线程提取数据库的数据,
   *
   * @param in_handle in_handle 具有消息组件,
   * @param in_filter 过滤器组件，筛选数据库中的数据
   *
   */
  explicit database_task(const entt::handle& in_handle, const database_n::filter& in_filter);
  ~database_task() override;
  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(base_type::delta_type, void* data);
};

}  // namespace doodle
