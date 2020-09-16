find_package(Qt5 COMPONENTS Core Sql REQUIRED)

set(HEADER_FILES
    "core_global.h"
    "src/filesqlinfo.h"
    "src/coresql.h"
    "src/coreset.h"
    "src/shotfilesqlinfo.h"
    "src/assfilesqlinfo.h"
    "src/episodes.h"
    "src/shot.h"
    "src/fileclass.h"
    "src/filetype.h"
    "src/asstype.h")

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
  )

target_link_libraries(core PRIVATE Qt5::Sql Qt5::Core )

target_compile_definitions(core PRIVATE CORE_LIBRARY)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

target_include_directories(core PUBLIC ${CMAKE_CURRENT_LIST_DIR})

install(TARGETS core DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include)
