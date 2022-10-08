target_sources(
  dingding_base
  PUBLIC
  lib_fixtures.h

  PRIVATE
  lib_fixtures.cpp
)
target_sources(
        test_core
        PUBLIC
        lib_fixtures.h

        PRIVATE
        lib_fixtures.cpp
)
target_sources(
        test_json_rpc
        PUBLIC
        lib_fixtures.h

        PRIVATE
        lib_fixtures.cpp
)
