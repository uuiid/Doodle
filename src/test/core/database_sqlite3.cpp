//
// Created by TD on 2023/1/13.
//
//
// Created by TD on 2022/8/26.
//

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/core/app_base.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/scan_data_t.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/detail/std_filesystem_path_orm.h>
#include <doodle_core/sqlite_orm/detail/uuid_to_blob.h>

#include <boost/test/unit_test.hpp>

using namespace doodle;

BOOST_AUTO_TEST_SUITE(sql_)

BOOST_AUTO_TEST_CASE(test_sqlite3_orm) {
  using namespace sqlite_orm;
  auto l_s = make_storage(
      "D:/test.db",  //
      make_table(
          "project_tab",                                                       //
          make_column("id", &project_helper::database_t::id_, primary_key()),  //
          make_column("uuid_id", &project_helper::database_t::uuid_id_, unique()),
          make_column("name", &project_helper::database_t::name_),      //
          make_column("en_str", &project_helper::database_t::en_str_),  //
          make_column("auto_upload_path", &project_helper::database_t::auto_upload_path_)
      )
  );
  l_s.sync_schema(true);

  project_helper::database_t l_d1{
      .id_               = 6,
      .uuid_id_          = core_set::get_set().get_uuid(),
      .name_             = "test",
      .en_str_           = "test",
      .auto_upload_path_ = "test"
  };
  project_helper::database_t l_d2{
      .uuid_id_          = core_set::get_set().get_uuid(),
      .name_             = "test2",
      .en_str_           = "test2",
      .auto_upload_path_ = "test2"
  };

  project_helper::database_t l_d3{
      .id_               = 6,
      .uuid_id_          = core_set::get_set().get_uuid(),
      .name_             = "test3",
      .en_str_           = "test3",
      .auto_upload_path_ = "test3"
  };
  l_s.replace(l_d3);
  l_d3.name_ = "test4";
  l_s.replace(l_d3);
}

// 多线程测试
BOOST_AUTO_TEST_CASE(multi_threaded) {
  struct Employee {
    int m_empno;
    std::string m_ename;
    std::string m_job;
    std::optional<int> m_mgr;
    std::string m_hiredate;
    double m_salary;
    std::optional<double> m_commission;
    int m_depno;
  };

  struct Department {
    int m_deptno;
    std::string m_deptname;
    std::string m_loc;
  };

  using namespace sqlite_orm;

  auto storage = make_storage(
      "D:/test.db",
      make_table(
          "Emp", make_column("empno", &Employee::m_empno, primary_key().autoincrement()),
          make_column("ename", &Employee::m_ename), make_column("job", &Employee::m_job),
          make_column("mgr", &Employee::m_mgr), make_column("hiredate", &Employee::m_hiredate),
          make_column("salary", &Employee::m_salary), make_column("comm", &Employee::m_commission),
          make_column("depno", &Employee::m_depno), foreign_key(&Employee::m_depno).references(&Department::m_deptno)
      ),
      make_table(
          "Dept", make_column("deptno", &Department::m_deptno, primary_key().autoincrement()),
          make_column("deptname", &Department::m_deptname), make_column("loc", &Department::m_loc, null())
      )
  );
  storage.sync_schema(true);
  storage.insert(Department{10, "Accounts", "New York"});
  storage.insert(Employee{1, "Paul", "Salesman", 2, "2002-02-12", 20000.0, 0.0, 1});
  storage.insert(Employee{2, "Allen", "Salesman", 2, "2002-02-12", 20000.0, 0.0, 1});
  storage.sync_schema(true);
}

struct test_1 {
  std::int32_t id_{};
  std::int32_t id_fk_{};
};

struct test_2 {
  std::int32_t id_{};
  std::string tt1{};
};

auto l_mk() {
  using namespace sqlite_orm;
  return make_storage(
      "C:/tes.db",  //
      make_table(
          "test_tab2",  //
          make_column("id", &test_1::id_, primary_key()), make_column("fk", &test_1::id_fk_),
          foreign_key(&test_1::id_fk_).references(&test_2::id_)
      ),

      make_table(
          "test_tab",  //
          make_column("id", &test_2::id_, primary_key()), make_column("tt1", &test_2::tt1, null())
      )
  );
}

BOOST_AUTO_TEST_CASE(tset_null) {
  {
    auto l_s = l_mk();
    l_s.sync_schema(true);
    auto l_id = l_s.insert(test_2{});
    l_s.insert(test_1{.id_fk_ = l_id});
  }
  auto l_s = l_mk();
  l_s.sync_schema(true);  // 这里出现错误
}

BOOST_AUTO_TEST_SUITE_END()