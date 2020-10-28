include(FindPackageHandleStandardArgs)

find_library(
        SQLPPSQLITE_LIBRARY
        NAMES sqlpp11-connector-sqlite3.lib)

#set(SQLPPMYSQL_LIBRARY $ENV{VCPKG_MYROOT}/vcpkg/installed/x64-windows/debug/lib/sqlpp-mysql.lib)

find_library(SQLITE_LIBRARY
        NAMES sqlite3.lib)

message("SQLPPSQLITE_LIBRARY ${SQLPPSQLITE_LIBRARY}")
message("SQLITE_LIBRARY ${SQLITE_LIBRARY}")

find_package_handle_standard_args(SqlppSqlite
        REQUIRED_VARS SQLPPSQLITE_LIBRARY SQLITE_LIBRARY
        )

add_library(SqlppSqlite STATIC IMPORTED GLOBAL)
set_target_properties(SqlppSqlite PROPERTIES
        IMPORTED_LOCATION "${SQLPPSQLITE_LIBRARY}"
        INTERFACE_LINK_LIBRARIES "${SQLITE_LIBRARY}")