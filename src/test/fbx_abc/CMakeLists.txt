find_package(Fbx 2020.3.4)

if (TARGET Fbx::Fbx)
    add_executable(abc2fbx abc2fbx.cpp)
    target_link_libraries(
            abc2fbx
            PRIVATE
            doodle_lib
            Fbx::Fbx-static
            Alembic::Alembic
    )
endif ()