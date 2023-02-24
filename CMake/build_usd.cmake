# set(FETCHCONTENT_FULLY_DISCONNECTED ON)
#set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
#
#include(ExternalProject)
#include(FetchContent)
#
#FetchContent_Declare(
#        tbbPackage
#        URL https://github.com/oneapi-src/oneTBB/releases/download/2019_U6/tbb2019_20190410oss_win.zip
#        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
#)
#FetchContent_MakeAvailable(tbbPackage)

#ExternalProject_Add(
#        UsdPackage
#        GIT_REPOSITORY https://github.com/PixarAnimationStudios/USD.git
#        GIT_TAG release
#        CMAKE_ARGS
#        -DTBB_ROOT_DIR=${tbbpackage_SOURCE_DIR}/tbb2019_20190410oss
#        -DPXR_BUILD_ALEMBIC_PLUGIN:BOOL=OFF
#        -DPXR_BUILD_EMBREE_PLUGIN:BOOL=OFF
#        -DPXR_BUILD_IMAGING:BOOL=OFF
#        -DPXR_BUILD_MONOLITHIC:BOOL=OFF
#        -DPXR_BUILD_TESTS:BOOL=OFF
#        -DPXR_BUILD_EXAMPLES:BOOL=OFF
#        -DPXR_BUILD_USD_IMAGING:BOOL=OFF
#        -DPXR_BUILD_TUTORIALS:BOOL=OFF
#        -DPXR_BUILD_USD_TOOLS:BOOL=OFF
#        -DPXR_ENABLE_PYTHON_SUPPORT:BOOL=OFF
#        -DBoost_NO_BOOST_CMAKE:BOOL=NO
#        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
#        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
#        --toolchain=${CMAKE_TOOLCHAIN_FILE}
#)
#ExternalProject_Get_Property(UsdPackage INSTALL_DIR)
#
#find_package(pxr CONFIG
#        HINTS ${INSTALL_DIR})
#FetchContent_Declare(
#        UsdPackage
#        GIT_REPOSITORY https://github.com/uuiid/USD.git
#        GIT_TAG release
#
#        CMAKE_ARGS -DTBB_ROOT_DIR=${tbbpackage_SOURCE_DIR}/tbb2019_20190410oss
#        -DPXR_BUILD_ALEMBIC_PLUGIN:BOOL=OFF
#        -DPXR_BUILD_EMBREE_PLUGIN:BOOL=OFF
#        -DPXR_BUILD_IMAGING:BOOL=OFF
#        -DPXR_BUILD_MONOLITHIC:BOOL=OFF
#        -DPXR_BUILD_TESTS:BOOL=OFF
#        -DPXR_BUILD_EXAMPLES:BOOL=OFF
#        -DPXR_BUILD_USD_IMAGING:BOOL=OFF
#        -DPXR_BUILD_TUTORIALS:BOOL=OFF
#        -DPXR_BUILD_USD_TOOLS:BOOL=OFF
#        -DPXR_ENABLE_PYTHON_SUPPORT:BOOL=OFF
#        -DBoost_NO_BOOST_CMAKE:BOOL=NO
#        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
#)
#set(TBB_ROOT_DIR ${tbbpackage_SOURCE_DIR}/tbb2019_20190410oss)
#set(PXR_BUILD_ALEMBIC_PLUGIN OFF)
#set(PXR_BUILD_EMBREE_PLUGIN OFF)
#set(PXR_BUILD_IMAGING OFF)
#set(PXR_BUILD_MONOLITHIC OFF)
#set(PXR_BUILD_TESTS OFF)
#set(PXR_BUILD_EXAMPLES OFF)
#set(PXR_BUILD_USD_IMAGING OFF)
#set(PXR_BUILD_TUTORIALS OFF)
#set(PXR_BUILD_USD_TOOLS OFF)
#
#FetchContent_MakeAvailable(UsdPackage)
#add_custom_target(genexdebug COMMAND ${CMAKE_COMMAND} -E echo "${PXR_USD_ROOT}$<$<CONFIG:Debug>:/debug>")
if (DEFINED PXR_USD_ROOT)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(PXR_USD_ROOT_ ${PXR_USD_ROOT}/debug)
    endif ()
    find_package(pxr CONFIG HINTS ${PXR_USD_ROOT})
    # $<$<CONFIG:Debug>:/debug>
    message("find pxr ${PXR_LIBRARIES}")
endif ()