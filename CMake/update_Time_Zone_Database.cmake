include(CMakePrintHelpers)

set(db_win ${ROOT_DIR}/src/doodle_lib/resource/tzdata/windowsZones.xml)
set(tzdata ${ROOT_DIR}/src/doodle_lib/resource/tzdata/tzdata-latest.tar.gz)
set(tzdata_dir ${ROOT_DIR}/src/doodle_lib/resource/tzdata/)

cmake_print_variables(
        ROOT_DIR
        db_win
        tzdata
        tzdata_dir)


if (EXISTS ${tzdata_dir})
    file(REMOVE_RECURSE ${tzdata_dir})
else ()
    file(MAKE_DIRECTORY ${tzdata_dir})
endif ()

file(DOWNLOAD
        https://raw.githubusercontent.com/unicode-org/cldr/master/common/supplemental/windowsZones.xml
        ${db_win}
        SHOW_PROGRESS)
file(DOWNLOAD
        https://www.iana.org/time-zones/repository/tzdata-latest.tar.gz
        ${tzdata}
        SHOW_PROGRESS)
execute_process(COMMAND ${CMAKE_COMMAND} -E tar xfz ${tzdata}
        WORKING_DIRECTORY ${tzdata_dir}
        RESULT_VARIABLE rv)
file(REMOVE ${tzdata})
