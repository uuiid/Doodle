#include "redirection_path_info.h"
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sql_com.h>
#include <doodle_core/generate/core/sql_sql.h>
#include <doodle_core/logger/logger.h>

#include "metadata/metadata.h"
#include "metadata/redirection_path_info.h"
#include <algorithm>
#include <entt/entity/fwd.hpp>
#include <lib_warp/enum_template_tool.h>
#include <magic_enum.hpp>
#include <sqlpp11/aggregate_functions/count.h>
#include <sqlpp11/insert.h>
#include <sqlpp11/parameter.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/update.h>
#include <string>

namespace doodle::database_n{
namespace sql=doodle_database;
void sql_com<doodle::redirection_path_info>::insert(conn_ptr &in_ptr, const entt::observer &in_observer){
    namespace uuids=boost::uuids;
    auto& l_conn =*in_ptr;
    auto l_handles =in_observer | ranges::views::transform([&](entt::entity in_entity) {
                     return entt::handle{*reg_, in_entity};
                   }) |
                   ranges::to_vector;
    sql::RedirectionPathInfo l_table{};

    auto l_pre=l_conn.prepare(sqlpp::insert_into(l_table).set(
        l_table.redirectionPath=sqlpp::parameter(l_table.redirectionPath),
        l_table.redirectionFileName=sqlpp::parameter(l_table.redirectionFileName),
        l_table.entityId=sqlpp::parameter(l_table.entityId)
    ));

    for(auto& l_h:l_handles){
        auto& l_r_p_i =l_h.get<redirection_path_info>();
        //todo:做一个updata
        // for(int i=0;i<l_r_p_i.search_path_.size();i++){
        //     l_pre.params.redirectionPath=l_r_p_i.search_path_[i].string();
        // }
       
        l_pre.params.redirectionFileName=l_r_p_i.file_name_.string();
        l_pre.params.entityId=boost::numeric_cast<std::int64_t>(l_h.get<database>().get_id());

        auto l_r              = l_conn(l_pre);
        DOODLE_LOG_INFO("更新数据库id {} -> 实体 {} 组件 {} ", l_r, l_h.entity(), rttr::type::get<redirection_path_info>().get_name());
    }
}
}