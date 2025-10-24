//
// Created by TD on 25-2-20.
//

#include "sqlite_database_impl.h"

#include <type_traits>
namespace doodle::details {
using namespace sqlite_orm;
template <class O, class T, class... Op>
using Column    = internal::column_t<T, const T& (O::*)() const, void (O::*)(T), Op...>;
using Storage   = internal::storage_t<internal::base_table<
      server_task_info, std::false_type,                                                                        //
      internal::column_t<decltype(&server_task_info::id_), internal::empty_setter, internal::primary_key_t<>>,  //
      internal::column_t<
          decltype(&server_task_info::uuid_id_), internal::empty_setter, internal::unique_t<>, internal::not_null_t>,  //
      internal::column_t<decltype(&server_task_info::exe_), internal::empty_setter>,  //
      internal::column_t<
          const std::string& (server_task_info::*)() const, void (server_task_info::*)(const std::string&)>  //
      >>;
using Storage_2 = decltype(make_storage(
    "", make_table(
            "test_tab", make_column("id", &server_task_info::id_, primary_key()),
            make_column("uuid_id", &server_task_info::uuid_id_, unique(), not_null()),  //
            make_column("exe", &server_task_info::exe_),                                //
            make_column(
                "command", static_cast<void (server_task_info::*)(const std::string&)>(&server_task_info::sql_command),
                static_cast<const std::string& (server_task_info::*)() const>(&server_task_info::sql_command)
            )
        )
));

static_assert(std::is_same_v<Storage, Storage_2>);

}  // namespace doodle::details