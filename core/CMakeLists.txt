find_package(Qt5 COMPONENTS Core Sql REQUIRED)

set(HEADER_FILES
    "core_global.h"
    "src/filesqlinfo.h"
    "src/coresql.h"
    "src/coreset.h")

add_library(core SHARED
    ${HEADER_FILES}
    "src/filesqlinfo.cpp"
    "src/coresql.cpp"
    "src/coreset.cpp"
  )

target_link_libraries(core PRIVATE Qt5::Sql Qt5::Core )

target_compile_definitions(core PRIVATE CORE_LIBRARY)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

target_include_directories(core PUBLIC ${CMAKE_CURRENT_LIST_DIR})

install(TARGETS core DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include)
