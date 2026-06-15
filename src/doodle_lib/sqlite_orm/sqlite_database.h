//
// Created by TD on 24-9-12.
//

#pragma once

#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/scan_data_t.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>

#include <boost/asio/awaitable.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include "sqlite_orm/orm/delete.h"
#include <optional>
#include <stdexcept>
#include <vector>

namespace doodle {
struct preview_file;
struct preview_files_for_entity_t;
enum class computer_status;
struct status_automation;
enum class server_task_info_type;
struct asset_type;
class server_task_info;
struct todo_t;
struct get_comments_t;
struct task_status;
struct assets_and_tasks_t;
struct entities_and_tasks_t;
struct ai_studio_person_role_link;
struct department;
struct comment;
struct task;
struct entity_link;
struct project_task_status_link;
struct entity_asset_extend;
struct playlist_shot;
struct entity_shot_extend;
struct task_type_asset_type_link;
struct working_file;
namespace attendance_helper {
struct database_t;
}
namespace assets_file_helper {
struct database_t;
struct link_parent_t;
}  // namespace assets_file_helper
namespace work_xlsx_task_info_helper {
struct database_t;
}
}  // namespace doodle
namespace doodle {

class sqlite_database : public orm::storage {
  using strand_type = boost::asio::strand<boost::asio::io_context::executor_type>;
  strand_type strand_{boost::asio::make_strand(g_io_context())};
  static constexpr std::size_t g_step_size{100};

  /**
   * 这回调函数用于加载数据库,  并且将数据库中的id分配到 sql_id 池中,  以便后续操作,
   * @warning 只有这里会分配id,  之后的操作不会分配, 只会查找id是否为 0 作为插入和更新的依据,
   * 并且在插入id的时候会自动更新为实际id
   * @param in_path 输入的数据库路径
   */
  void open_(FSys::path in_path, std::int32_t in_flags) override;
  void register_custom_extension(sqlite3* in_sqlite) override;

 public:
  std::vector<uuid> get_temporal_type_ids();

 public:
  sqlite_database()  = default;
  ~sqlite_database() = default;

  void regs_all();

  /// 备份数据库
  boost::asio::awaitable<void> backup(FSys::path in_path);

  template <typename T>
  std::vector<T> get_all() {
    using namespace orm;
    return select(*this).columns(object<T>()).template from<T>()().to_vector();
  }

  template <typename T>
  std::int64_t uuid_to_id(uuid in_uuid) {
    using namespace orm;
    return select(*this).columns(&T::id_).template from<T>().where(c(&T::uuid_id_) == in_uuid)().to_optional().value_or(
        0
    );
  }

  template <typename T>
  uuid id_to_uuid(std::int64_t in_id) {
    using namespace orm;
    return select(*this)
        .columns(&T::uuid_id_)
        .template from<T>()
        .where(c(&T::id_) == in_id)()
        .to_optional()
        .value_or(uuid{});
  }

  template <typename T>
  T get_by_uuid(uuid in_uuid) {
    using namespace orm;
    return select(*this).columns(object<T>()).template from<T>().where(c(&T::uuid_id_) == in_uuid)().to_single();
  }
#define DOODLE_TO_SQLITE_THREAD()                                     \
  DOODLE_CHICK(!core_set::get_set().read_only_mode_, "只读不可保存"); \
  auto this_executor = co_await boost::asio::this_coro::executor;     \
  co_await boost::asio::dispatch(boost::asio::bind_executor(strand_, boost::asio::use_awaitable));

  /// 测试成员字段 uuid_id_ 是否存在，以及是否是 uuid 类型
  template <typename T, typename = void>
  struct has_uuid_id_impl : std::false_type {};

  template <typename T>
  struct has_uuid_id_impl<T, std::enable_if_t<std::is_same_v<uuid, decltype(std::declval<T>().uuid_id_)>>>
      : std::true_type {};

  template <typename T>
  static constexpr bool has_uuid_id = has_uuid_id_impl<T>::value;
  /// 测试成员字段 created_at_ 是否存在，以及是否是 chrono::system_zoned_time 类型
  template <typename T, typename = void>
  struct has_created_at_impl : std::false_type {};

  template <typename T>
  struct has_created_at_impl<
      T, std::enable_if_t<std::is_same_v<chrono::system_zoned_time, decltype(std::declval<T>().created_at_)>>>
      : std::true_type {};

  template <typename T>
  static constexpr bool has_created_at = has_created_at_impl<T>::value;
  /// 测试成员字段 updated_at_ 是否存在，以及是否是 chrono::system_zoned_time 类型
  template <typename T, typename = void>
  struct has_updated_at_impl : std::false_type {};

  template <typename T>
  struct has_updated_at_impl<
      T, std::enable_if_t<std::is_same_v<chrono::system_zoned_time, decltype(std::declval<T>().updated_at_)>>>
      : std::true_type {};

  template <typename T>
  static constexpr bool has_updated_at = has_updated_at_impl<T>::value;

  template <typename T>
  boost::asio::awaitable<void> install(std::shared_ptr<T> in_data) {
    using namespace orm;
    DOODLE_CHICK(in_data, "不可传入空指针");
    if constexpr (has_uuid_id<T>) {
      DOODLE_CHICK(in_data->uuid_id_.is_nil(), "传入的数据实体 uuid_id_ 不为空");
      in_data->uuid_id_ = core_set::get_set().get_uuid();
    }
    DOODLE_CHICK(in_data->id_ == 0, "必须传入id为0的新实体");

    if constexpr (has_created_at<T>)
      in_data->created_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};

    if constexpr (has_updated_at<T>)
      in_data->updated_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};

    DOODLE_TO_SQLITE_THREAD();
    install_unsafe<T>(in_data);
    DOODLE_TO_SELF();
  }
  template <typename T>
  boost::asio::awaitable<void> update(std::shared_ptr<T> in_data) {
    DOODLE_CHICK(in_data, "不可传入空指针");
    if constexpr (has_uuid_id<T>) {
      DOODLE_CHICK(!in_data->uuid_id_.is_nil(), "传入的数据实体 uuid_id_ 为空");
      in_data->id_ = uuid_to_id<T>(in_data->uuid_id_);
    }
    DOODLE_CHICK(in_data->id_ != 0, "不可传入空指针");

    if constexpr (has_updated_at<T>)
      in_data->updated_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};

    DOODLE_TO_SQLITE_THREAD();
    update_unsafe<T>(in_data);
    DOODLE_TO_SELF();
  }
  template <typename T>
  void install_unsafe(std::shared_ptr<T> in_data) {
    using namespace orm;
    in_data->id_ = orm::insert(*this).into<T>().values(object<T>(*in_data))();
  }

  template <typename T>
  void update_unsafe(std::shared_ptr<T> in_data) {
    auto l_g = transaction();
    using namespace orm;
    orm::update(*this).from<T>().set(object<T>(*in_data)).where(c(&T::id_) == in_data->id_)();
    l_g.commit();
  }
  template <typename T>
  void update_sync(std::shared_ptr<T> in_data) {
    DOODLE_CHICK(in_data, "不可传入空指针");
    if constexpr (has_uuid_id<T>) {
      DOODLE_CHICK(!in_data->uuid_id_.is_nil(), "传入的数据实体 uuid_id_ 为空");
      in_data->id_ = uuid_to_id<T>(in_data->uuid_id_);
    }
    DOODLE_CHICK(in_data->id_ != 0, "不可传入空指针");

    if constexpr (has_updated_at<T>)
      in_data->updated_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};
    boost::asio::post(strand_, [this, in_data = std::move(in_data)]() mutable { update_unsafe<T>(in_data); });
  }

  /**
   *
   * @tparam T 任意优化类别
   * @param in_data 传入的数据
   * @return 插入的id(不包含更新的id)
   */
  template <typename T>
  boost::asio::awaitable<void> install_range(std::shared_ptr<std::vector<T>> in_data) {
    DOODLE_CHICK(in_data, "不可传入空指针");
    if (in_data->empty()) co_return;
    auto l_id_is_zero = std::ranges::all_of(*in_data, [](const auto& in_) { return in_.id_ == 0; });
    DOODLE_CHICK(l_id_is_zero, "传入的数据实体 id_ 必须全部为0");
    if constexpr (has_uuid_id<T>)
      for (auto& in_ : *in_data) in_.uuid_id_ = core_set::get_set().get_uuid();

    if constexpr (has_created_at<T>)
      for (auto& in_ : *in_data)
        in_.created_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};
    if constexpr (has_updated_at<T>)
      for (auto& in_ : *in_data)
        in_.updated_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};

    DOODLE_TO_SQLITE_THREAD();

    auto l_g    = transaction();
    auto l_size = in_data->size();
    using namespace orm;
    auto l_insert = orm::insert(*this).into<T>();
    bool first    = true;
    for (auto&& view : *in_data | ranges::views::chunk(g_step_size)) {
      if (first) {
        l_insert.set_range(view)();
        first = false;
      } else {
        try {
          l_insert.rebind_range(view)();
        } catch (const rebind_range_size_mismatch_exception& e) {
          SPDLOG_ERROR("安装范围失败: {}", e.what());
          orm::insert(*this).into<T>().set_range(view)();
        }
      }
    }
    l_g.commit();
    DOODLE_TO_SELF();

    if constexpr (has_uuid_id<T>) {
      std::map<uuid, std::int64_t> l_id_map{};
      std::vector<uuid> l_uuids = *in_data | ranges::views::transform([](const auto& in_) { return in_.uuid_id_; }) |
                                  ranges::to<std::vector<uuid>>();

      for (auto&& [key, val] :
           select(*this).columns(&T::id_, &T::uuid_id_).template from<T>().where(c(&T::uuid_id_).in(l_uuids))()) {
        l_id_map[val] = key;
      }
      for (std::size_t i = 0; i < l_size; ++i) {
        (*in_data)[i].id_ = l_id_map[(*in_data)[i].uuid_id_];
      }
    }
  }
  template <typename T>
  boost::asio::awaitable<void> update_range(std::shared_ptr<std::vector<T>> in_data) {
    DOODLE_CHICK(in_data, "不可传入空指针");
    if (in_data->empty()) co_return;
    auto l_id_not_zero = std::ranges::all_of(*in_data, [](const auto& in_) { return in_.id_ != 0; });
    DOODLE_CHICK(l_id_not_zero, "传入的数据实体 id_ 不可为0");

    if constexpr (has_updated_at<T>)
      for (auto& in_ : *in_data)
        in_.updated_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};

    DOODLE_TO_SQLITE_THREAD();
    auto l_g = transaction();
    using namespace orm;
    auto l_update = orm::update(*this).from<T>();
    for (auto l_is_begin = true; auto&& i : *in_data) {
      if (l_is_begin) {
        l_is_begin = false;
        l_update.set(object<T>(i))();
      } else
        l_update.rebind(object<T>(i))();
    }
    l_g.commit();
    DOODLE_TO_SELF();
  }

  template <typename T>
  boost::asio::awaitable<void> remove(std::vector<std::int64_t> in_data) {
    DOODLE_TO_SQLITE_THREAD();
    if (in_data.empty()) co_return;
    auto l_g = transaction();
    for (auto i = 0; i < in_data.size();) {
      auto l_end = std::min(i + g_step_size, in_data.size());
      std::vector<std::int64_t> l_v{in_data.begin() + i, in_data.begin() + l_end};
      using namespace orm;
      delete_from(*this).from<T>().where(c(&T::id_).in(l_v))();
      i = l_end;
    }
    l_g.commit();
    DOODLE_TO_SELF();
  }
  template <typename T>
  boost::asio::awaitable<void> remove(std::vector<std::int32_t> in_data) {
    DOODLE_TO_SQLITE_THREAD();
    if (in_data.empty()) co_return;
    auto l_g = transaction();
    for (auto i = 0; i < in_data.size();) {
      auto l_end = std::min(i + g_step_size, in_data.size());
      std::vector<std::int64_t> l_v{in_data.begin() + i, in_data.begin() + l_end};
      using namespace orm;
      delete_from(*this).from<T>().where(c(&T::id_).in(l_v))();
      i = l_end;
    }
    l_g.commit();
    DOODLE_TO_SELF();
  }
  template <typename T>
  boost::asio::awaitable<void> remove(std::int64_t in_data) {
    DOODLE_TO_SQLITE_THREAD();
    using namespace orm;
    delete_from(*this).from<T>().where(c(&T::id_) == in_data)();
    DOODLE_TO_SELF();
  }
  template <typename T>
  boost::asio::awaitable<void> remove(std::vector<uuid> in_data) {
    DOODLE_TO_SQLITE_THREAD();
    if (in_data.empty()) co_return;
    auto l_g = transaction();
    for (auto i = 0; i < in_data.size();) {
      auto l_end = std::min(i + g_step_size, in_data.size());
      std::vector<uuid> l_v{in_data.begin() + i, in_data.begin() + l_end};
      using namespace orm;
      delete_from(*this).from<T>().where(c(&T::uuid_id_).in(l_v))();
      i = l_end;
    }
    l_g.commit();
    DOODLE_TO_SELF();
  }
  template <typename T>
  boost::asio::awaitable<void> remove(uuid in_data) {
    DOODLE_TO_SQLITE_THREAD();
    using namespace orm;
    delete_from(*this).from<T>().where(c(&T::uuid_id_) == in_data)();
    DOODLE_TO_SELF();
  }

  boost::asio::awaitable<void> remove(orm::delete_t in_delete);
  boost::asio::awaitable<void> update(orm::update_t in_update);

  boost::asio::awaitable<void> mark_all_notifications_as_read(uuid in_user_id);

  std::vector<attendance_helper::database_t> get_attendance(
      const uuid& in_person_id, const chrono::local_days& in_data
  );
  std::vector<attendance_helper::database_t> get_attendance(
      const uuid& in_person_id, const std::vector<chrono::local_days>& in_data
  );
  std::vector<work_xlsx_task_info_helper::database_t> get_work_xlsx_task_info(
      const uuid& in_person_id, const chrono::local_days& in_data
  );

  person get_person_for_email(const std::string& in_email);
  /// 获取用户所在的团队对应的项目
  std::vector<project> get_person_projects(const person& in_user);

  std::optional<project_task_type_link> get_project_task_type_link(
      const uuid& in_project_id, const uuid& in_task_type_id
  );
  std::optional<project_task_status_link> get_project_task_status_link(
      const uuid& in_project_id, const uuid& in_task_status_uuid
  );
  std::optional<project_asset_type_link> get_project_asset_type_link(
      const uuid& in_project_id, const uuid& in_asset_type_uuid
  );
  std::vector<person> get_project_persons(const uuid& in_project_uuid);
  // 查询人员是否在项目团队中
  bool is_person_in_project(const person& in_person, const uuid& in_project_id);
  bool is_person_in_project(const uuid& in_person, const uuid& in_project_id);
  // 查询对应的task是否存在
  bool is_task_exist(const uuid& in_entity_id, const uuid& in_task_type_id);

  task_status get_task_status_by_name(const std::string& in_name);
  asset_type get_entity_type_by_name(const std::string& in_name);

  // 获取用户的订阅
  std::set<uuid> get_person_subscriptions(
      const person& in_person, const uuid& in_project_id, const uuid& in_asset_type_uuid
  ) {
    return get_person_subscriptions(in_person.uuid_id_, in_project_id, {in_asset_type_uuid});
  }
  std::set<uuid> get_person_subscriptions(
      const uuid& in_person, const uuid& in_project_id, const std::vector<uuid>& in_asset_type_uuid
  );

  std::set<uuid> get_notification_recipients(const task& in_task);
  std::set<uuid> get_mentioned_people(const uuid& project_id, const comment& in_comment_id);
  std::vector<status_automation> get_project_status_automations(const uuid& in_project_uuid);
  /// 返回一个允许将任务类型id与优先级匹配的字典。
  std::map<uuid, std::int32_t> get_task_type_priority_map(const uuid& in_project, const std::string& in_for_entity);
  /// 返回所属实体,类别 的任务
  std::optional<task> get_tasks_for_entity_and_task_type(const uuid& in_entity_id, const uuid& in_task_type_id);
  /// 在模型库中, 是否有和资产类别关联的模型
  bool has_assets_tree_assets_link(const uuid& in_label_uuid);
  /// 是否存在 label_assets_link
  bool has_assets_tree_assets_link(const uuid& in_label_uuid, const uuid& in_asset_uuid);
  /// 模型库中,是否有资产类别的子类别
  bool has_assets_tree_child(const uuid& in_label_uuid);
  /// 获取资产类别和模型的连接
  assets_file_helper::link_parent_t get_assets_tree_assets_link(const uuid& in_label_uuid, const uuid& in_asset_uuid);
  ///
  std::map<uuid, std::vector<preview_files_for_entity_t>> get_preview_files_for_entity(const uuid& in_entity_id);
  /// 获取评论对应的预览图
  std::optional<preview_file> get_preview_file_for_comment(const uuid& in_comment_id);
  /// 是否将任务分配给了用户
  bool is_task_assigned_to_person(const uuid& in_task, const uuid& in_person);

  /// 获取任务的下一个预览版本号
  std::int64_t get_next_preview_revision(const uuid& in_task_id);
  /// 当前评论是否有预览图
  bool has_preview_file(const uuid& in_comment);
  /// 获取评论的预览图位置(一条评论中可能有多个预览图, 图片序列)
  std::int64_t get_next_position(const uuid& in_task_id, const std::int64_t& in_revision);
  /// 获取评论预览图对应的版本号
  std::int64_t get_preview_revision(const uuid& in_comment);
  /// 获取task中的最后一条评论
  std::optional<comment> get_last_comment(const uuid& in_task_id);
  /// 获取资产对应的 task
  std::vector<task> get_tasks_for_entity(const uuid& in_asset_id);
  std::vector<asset_type> get_asset_types_not_temporal_type();
  std::optional<entity_link> get_entity_link(const uuid& in_entity_in_id, const uuid& in_asset_id);
  /// 获取任务额外数据
  std::optional<entity_asset_extend> get_entity_asset_extend(const uuid& in_entity_id);
  /// 获取镜头扩展数据
  std::optional<entity_shot_extend> get_entity_shot_extend(const uuid& in_entity_id);
  /// 获取播放序列对应的实体
  std::vector<playlist_shot> get_playlist_shot_entity(const uuid& in_playlist_id);

  boost::asio::awaitable<void> remove_playlist_shot_for_playlist(const uuid& in_playlist_id);
  std::optional<task_type_asset_type_link> get_task_type_asset_type_link(
      const uuid& in_task_type_id, const uuid& in_asset_type_id
  );
  boost::asio::awaitable<void> remove_task_type_asset_type_link_by_asset_type(const uuid& in_asset_type_id);
  uuid get_project_status_open();
  uuid get_project_status_closed();

  // 获取 项目中实体数量
  std::size_t get_project_entity_count(const uuid& in_project_id);

  // 是给外包授权的实体
  bool is_entity_outsourced(const uuid& in_entity_id, const uuid& in_studio_id, const uuid& in_parent_id = uuid{});
  // 删除sequence下的所有casting数据
  boost::asio::awaitable<void> remove_sequence_casting(const uuid& in_sequence_id);
  // 按照计算机id 获取工作
  std::vector<server_task_info> get_server_tasks_by_submitted();
  // 获取镜头任务对应的 场景资产的扩展数据 如果没有, 抛出异常, 大于一个, 抛出异常
  entity_asset_extend get_entity_shot_extend_by_task(const uuid& in_shot_id);
  // 更新计算机状态
  boost::asio::awaitable<void> update_computer_status(const uuid& in_computer_id, computer_status in_status);
  // 检查人员和ai工作室是否有连接
  bool is_person_ai_studio_connected(const uuid& in_person_id, const uuid& in_ai_studio_id);
  std::optional<ai_studio_person_role_link> get_ai_studio_person_role_link(
      const uuid& in_person_id, const uuid& in_ai_studio_id
  );
};
}  // namespace doodle