find_package(Qt5 COMPONENTS Core Sql REQUIRED)
find_package(Sqlpp11 REQUIRED)

#find_library(sqlpp-mysql_LIBS
#    NAMES sqlpp-mysql
#    PATHS  ${CMAKE_FIND_ROOT_PATH}/debug/lib
#    REQUIRED)

#find_library(mysql_LIBS
#    NAMES libmysql
#    PATHS  ${CMAKE_FIND_ROOT_PATH}/debug/lib
#    REQUIRED)

set(ORM_HEADER
    src/coreOrm/assclass_sqlOrm.h
    src/coreOrm/basefile_sqlOrm.h
    src/coreOrm/configure_sqlOrm.h
    src/coreOrm/episodes_sqlOrm.h
    src/coreOrm/fileclass_sqlOrm.h
    src/coreOrm/filetype_sqlOrm.h
    src/coreOrm/mainshot_sqlOrm.h
    src/coreOrm/shot_sqlOrm.h
    src/coreOrm/znch_sqlOrm.h
    )
file(COPY ${PROJECT_SOURCE_DIR}/sql-builder/sql.h
    DESTINATION ${CMAKE_CURRENT_LIST_DIR}/src/sql_builder)

set(HEADER_FILES
    "core_global.h"
    "src/sql_builder/sql.h"
    "src/filesqlinfo.h"
    "src/coresql.h"
    "src/coreset.h"
    "src/shotfilesqlinfo.h"
    "src/assfilesqlinfo.h"
    "src/episodes.h"
    "src/shot.h"
    "src/fileclass.h"
    "src/filetype.h"
    "src/asstype.h"
    "src/coresqldata.h")

add_library(core SHARED
    ${HEADER_FILES}
    "src/filesqlinfo.cpp"
    "src/coresql.cpp"
    "src/coreset.cpp"
    "src/shotfilesqlinfo.cpp"
    "src/assfilesqlinfo.cpp"
    "src/episodes.cpp"
    "src/shot.cpp"
    "src/fileclass.cpp"
    "src/filetype.cpp"
    "src/asstype.cpp"
    "src/coresqldata.cpp"
  )

target_link_libraries(core PUBLIC Qt5::Sql Qt5::Core  )

target_compile_definitions(core PRIVATE CORE_LIBRARY)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

target_include_directories(core PUBLIC ${CMAKE_CURRENT_LIST_DIR})

install(TARGETS core DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include)
