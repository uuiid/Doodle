include(doodle_exe)

add_subdirectory(gen)
add_subdirectory(limited_exe)

if(BUILD_LIMITED_1)
    add_subdirectory(task_exe)
else()
    add_subdirectory(GUI)
endif()
