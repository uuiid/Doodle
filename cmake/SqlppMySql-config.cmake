include(FindPackageHandleStandardArgs)

find_library(
        SQLPPMYSQL_LIBRARY
        NAMES sqlpp-mysql libsqlpp-mysql)

#set(SQLPPMYSQL_LIBRARY $ENV{VCPKG_MYROOT}/vcpkg/installed/x64-windows/debug/lib/sqlpp-mysql.lib)

find_library(MySql_LIBRARY
        NAMES libmariadb
        )

message("SQLPPMYSQL_LIBRARY ${SQLPPMYSQL_LIBRARY}")
message("MySql_LIBRARY ${MySql_LIBRARY}")

find_package_handle_standard_args(SqlppMySql
        REQUIRED_VARS SQLPPMYSQL_LIBRARY MySql_LIBRARY
        )

add_library(SqlppMySql STATIC IMPORTED GLOBAL)
set_target_properties(SqlppMySql PROPERTIES
        IMPORTED_LOCATION "${SQLPPMYSQL_LIBRARY}"
        INTERFACE_LINK_LIBRARIES "${MySql_LIBRARY}")
