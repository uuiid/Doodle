//
// Created by TD on 2022/5/30.
//

#include "select.h"

#include <doodle_core/core/core_sql.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_core/lib_warp/enum_template_tool.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/detail/time_point_info.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/metadata_cpp.h>
#include <doodle_core/metadata/redirection_path_info.h>
#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_task.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>

#include "details/tool.h"
#include "metadata/project.h"
#include <range/v3/all.hpp>
#include <range/v3/range.hpp>
#include <range/v3/range_for.hpp>
#include <sqlpp11/ppgen.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::database_n {
template <class t>
struct future_data {
  using id_map_type                                 = std::map<std::int64_t, entt::entity>;

  future_data()                                     = default;
  future_data(const future_data&)                   = delete;
  future_data& operator=(const future_data&)        = delete;
  future_data(future_data&& in) noexcept            = default;
  future_data& operator=(future_data&& in) noexcept = default;

  std::vector<std::tuple<std::int64_t, std::future<t>>> data{};

  void install_reg(const registry_ptr& in_reg, const id_map_type& in_map_type) {
    std::vector<entt::entity> l_entt_list{};
    std::set<entt::entity> l_entt_set;
    std::vector<t> l_data_list{};
    std::vector<entt::entity> l_not_valid_entity;
    std::vector<entt::entity> l_duplicate_entity;
    for (auto&& [l_id, l_t] : data) {
      /// \brief 保证id 具有对应的实体
      if (in_map_type.find(l_id) == in_map_type.end()) {
        l_not_valid_entity.emplace_back(num_to_enum<entt::entity>(l_id));
        continue;
      }
      auto l_entt = in_map_type.at(l_id);
      /// \brief 保证实体已经注册
      if (!in_reg->valid(l_entt)) {
        l_not_valid_entity.emplace_back(l_entt);
        continue;
      }
      /// \brief 保证实体不重复
      if (l_entt_set.find(l_entt) == l_entt_set.end()) {
        l_entt_list.push_back(l_entt);
        l_data_list.emplace_back(std::move(l_t.get()));
      } else {
        l_duplicate_entity.emplace_back(l_entt);
      }
      l_entt_set.emplace(l_entt);
    }
    if (!l_not_valid_entity.empty()) {
      DOODLE_LOG_WARN("{} 无效的实体: {} 重复的实体 {}", typeid(t).name(), l_not_valid_entity, l_duplicate_entity);
      // for (auto&& d : l_not_valid_entity) {
      //   doodle::database::delete_(entt::handle{*in_reg, d});
      // }
    }
    DOODLE_CHICK(
        ranges::all_of(l_entt_list, [&](const entt::entity& in) { return in_reg->valid(in); }), doodle_error{"无效实体"}
    );
    in_reg->remove<t>(l_entt_list.begin(), l_entt_list.end());
    in_reg->insert<t>(l_entt_list.begin(), l_entt_list.end(), l_data_list.begin());
  };
};

class select::impl {
 public:
  using boost_strand = boost::asio::strand<std::decay_t<decltype(g_thread())>::executor_type>;
  using id_map_type  = std::map<std::int64_t, entt::entity>;
  /**
   * 数据库的绝对路径
   */
  FSys::path project;
  bool only_ctx{false};
  std::future<void> result;
  std::vector<std::shared_future<void>> results;
  std::atomic_bool stop{false};
  boost_strand strand_{boost::asio::make_strand(g_thread())};
  std::vector<boost_strand> strands_{};

  registry_ptr local_reg{g_reg()};

  std::vector<std::function<void(const registry_ptr&)>> list_install{};

  std::vector<entt::entity> create_entt{};
  id_map_type id_map{};
  process_message* process_message_{};
#if defined(DOODLE_SQL_compatible_v2)
#pragma region "old compatible 兼容旧版函数"
  void select_old(entt::registry& in_reg, sqlpp::sqlite3::connection& in_conn) {
    if (auto [l_v, l_i] = doodle::database_n::details::get_version(in_conn);
        l_v == 3 && l_i >= 4 && doodle::database_n::details::db_compatible::has_metadatatab_table(in_conn)) {
      Metadatatab l_metadatatab{};

      std::size_t l_size{1};
      for (auto&& raw : in_conn(sqlpp::select(sqlpp::count(l_metadatatab.id)).from(l_metadatatab).unconditionally())) {
        l_size = raw.count.value();
        break;
      }

      for (auto&& row : in_conn(sqlpp::select(sqlpp::all_of(l_metadatatab)).from(l_metadatatab).unconditionally())) {
        auto l_e = num_to_enum<entt::entity>(row.id.value());
        if (!in_reg.valid(l_e)) l_e = in_reg.create(l_e);

        auto l_fun = boost::asio::post(
            strand_, std::packaged_task<void()>{[l_e, in_str = row.userData.value(), in_uuid = row.uuidData.value(),
                                                 &in_reg, l_size, this]() {
              if (stop) return;
              entt::handle l_h{in_reg, l_e};
              l_h.emplace<database>(in_uuid).set_id(0);
              auto k_json = nlohmann::json::parse(in_str);
              entt_tool::load_comm<
                  doodle::project, doodle::episodes, doodle::shot, doodle::season, doodle::assets, doodle::assets_file,
                  doodle::time_point_wrap, doodle::comment, doodle::project_config::base_config, doodle::image_icon,
                  doodle::importance, doodle::redirection_path_info>(l_h, k_json);
              database::save(l_h);
              process_message_->message("开始旧版本兼容转换"s);
              process_message_->progress_step({1, l_size});
            }}
        );
        results.emplace_back(l_fun.share());
        if (stop) return;
      }
      auto l_fun = boost::asio::post(strand_, std::packaged_task<void()>{[this]() {
                                       auto l_view = local_reg->view<doodle::project>();
                                       if (!l_view.empty()) {
                                         auto l_h = entt::handle{*local_reg, l_view.front()};
                                         local_reg->ctx().at<doodle::project>() = l_h.get<doodle::project>();
                                         local_reg->ctx().at<doodle::project_config::base_config>() =
                                             l_h.any_of<doodle::project_config::base_config>()
                                                 ? l_h.get<doodle::project_config::base_config>()
                                                 : doodle::project_config::base_config{};
                                       }
                                       process_message_->message("完成旧版本兼容转换"s);
                                       process_message_->progress_clear();
                                     }});
      results.emplace_back(l_fun.share());
    }
  }
#pragma endregion
#endif
  template <typename Type>
  void _select_com_(entt::registry& in_reg, sqlpp::sqlite3::connection& in_conn) {
    tables::com_entity l_com_entity{};

    auto&& l_s = strands_.emplace_back(boost::asio::make_strand(g_thread()));
    std::size_t l_size{1};
    for (auto&& raw : in_conn(sqlpp::select(sqlpp::count(l_com_entity.id)).from(l_com_entity).unconditionally())) {
      l_size = raw.count.value();
      break;
    }

    auto l_future_data = std::make_shared<future_data<Type>>();

    for (auto&& row : in_conn(sqlpp::select(l_com_entity.entity_id, l_com_entity.json_data)
                                  .from(l_com_entity)
                                  .where(l_com_entity.com_hash == entt::type_id<Type>().hash()))) {
      if (stop) return;
      auto l_id  = row.entity_id.value();
      auto l_fut = boost::asio::post(
          l_s, std::packaged_task<Type()>{[in_json = row.json_data.value(), in_id = l_id, l_size, this]() {
            auto l_json = nlohmann::json::parse(in_json);
            process_message_->progress_step({1, l_size * 2});
            return l_json.template get<Type>();
          }}
      );

      l_future_data->data.emplace_back(std::make_tuple(boost::numeric_cast<std::int64_t>(l_id), std::move(l_fut)));
    }
    list_install.emplace_back([l_future_data = std::move(l_future_data), this](const registry_ptr& in) mutable {
      return l_future_data->install_reg(in, id_map);
    });
  }
  template <typename... Type>
  void select_com(entt::registry& in_reg, sqlpp::sqlite3::connection& in_conn) {
    (_select_com_<Type>(in_reg, in_conn), ...);
  }

  void select_entt(entt::registry& in_reg, sqlpp::sqlite3::connection& in_conn) {
    tables::entity l_entity{};

    std::size_t l_size{1};
    for (auto&& raw : in_conn(sqlpp::select(sqlpp::count(l_entity.id)).from(l_entity).unconditionally())) {
      l_size = raw.count.value();
      break;
    }
    auto l_future_data = std::make_shared<future_data<database>>();

    for (auto& row : in_conn(sqlpp::select(sqlpp::all_of(l_entity)).from(l_entity).unconditionally())) {
      if (stop) return;
      auto l_e = num_to_enum<entt::entity>(row.id.value());
      create_entt.push_back(l_e);

      auto l_fut = boost::asio::post(
          strand_,
          std::packaged_task<database()>{[in_json = row.uuid_data.value(), in_id = row.id, l_size, this]() -> database {
            database l_database{in_json};
            l_database.set_id(in_id);
            process_message_->progress_step({1, l_size * 2});
            return l_database;
          }}
      );
      l_future_data->data.emplace_back(boost::numeric_cast<std::int64_t>(enum_to_num(l_e)), std::move(l_fut));
    }

    list_install.emplace_back([l_future_data = std::move(l_future_data), this](const registry_ptr& in) mutable {
      return l_future_data->install_reg(in, id_map);
    });
  }
};

select::select() : p_i(std::make_unique<impl>()) {}
select::~select() = default;

namespace {
#include "details/macro.h"

DOODLE_SQL_TABLE_IMP(context, tables::column::id, tables::column::com_hash, tables::column::json_data);

void _select_ctx_(
    entt::registry& in_reg, sqlpp::sqlite3::connection& in_conn,
    const std::map<std::uint32_t, std::function<void(entt::registry& in_reg, const std::string& in_str)>>& in_fun_list
) {
  context l_context{};

  for (auto&& row : in_conn(sqlpp::select(l_context.com_hash, l_context.json_data).from(l_context).unconditionally())) {
    if (auto l_f = in_fun_list.find(row.com_hash.value()); l_f != in_fun_list.end()) {
      in_fun_list.at(row.com_hash.value())(in_reg, row.json_data.value());
    }
  }
}
template <typename... Type>
void select_ctx_template(entt::registry& in_reg, sqlpp::sqlite3::connection& in_conn) {
  std::map<std::uint32_t, std::function<void(entt::registry & in_reg, const std::string& in_str)>> l_fun{
      std::make_pair(entt::type_id<Type>().hash(), [&](entt::registry& in_reg, const std::string& in_str) {
        auto l_h    = entt::handle{in_reg, in_reg.create()};
        auto l_json = nlohmann::json::parse(in_str);
        l_h.emplace<Type>(std::move(l_json.get<Type>()));
      })...};

  _select_ctx_(in_reg, in_conn, l_fun);
}

}  // namespace

bool select::operator()(entt::registry& in_registry, const FSys::path& in_project_path, conn_ptr& in_connect) {
  p_i->process_message_ = g_reg()->ctx().find<process_message>();
  p_i->only_ctx         = false;
  p_i->project          = in_project_path;
#if defined(DOODLE_SQL_compatible_v2)
  this->p_i->select_old(*p_i->local_reg, *in_connect);
  /// \brief 等待旧的任务完成
  ranges::for_each(p_i->results, [](const decltype(p_i->results)::value_type& in_) { in_.get(); });
  p_i->results.clear();
#endif
  if (!detail::has_table(tables::com_entity{}, *in_connect)) return false;

  /// \brief 选中实体
  p_i->select_entt(*p_i->local_reg, *in_connect);
  /// \brief 等待实体创建完成

  /// @brief 选中组件
  p_i->select_com<DOODLE_SQLITE_TYPE>(*p_i->local_reg, *in_connect);

  /// \brief 选中上下文
  select_ctx_template<DOODLE_SQLITE_TYPE_CTX>(in_registry, *in_connect);

  /// \brief 开始修改注册表
  auto l_id = p_i->create_entt;
  p_i->local_reg->create(p_i->create_entt.begin(), p_i->create_entt.end());
  for (int l_j = 0; l_j < l_id.size(); ++l_j) {
    p_i->id_map.emplace(boost::numeric_cast<std::int64_t>(enum_to_num(l_id[l_j])), p_i->create_entt[l_j]);
  }

  for (auto&& l_f : p_i->list_install) {
    l_f(p_i->local_reg);
  }
  (*in_connect)(sqlpp::sqlite3::drop_if_exists_table(tables::com_entity{}));
  return true;
}

}  // namespace doodle::database_n
