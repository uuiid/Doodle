# usd 构建

``` cmd
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

set TBB_LOCATION=F:\source\install_tbb\tbb2019_20190410oss\lib\intel64\vc14

"C:\Program Files\CMake\bin\cmake.exe"^
 -G "Visual Studio 17 2022" -S %~dp0.. -B %~dp0/b_debug^
 -DTBB_ROOT_DIR=F:\source\install_tbb^
 -DPXR_BUILD_ALEMBIC_PLUGIN:BOOL=OFF^
 -DPXR_BUILD_EMBREE_PLUGIN:BOOL=OFF^
 -DPXR_BUILD_IMAGING:BOOL=OFF^
 -DPXR_BUILD_MONOLITHIC:BOOL=OFF^
 -DPXR_BUILD_TESTS:BOOL=OFF^
 -DPXR_BUILD_EXAMPLES=OFF^
 -DPXR_BUILD_USD_IMAGING:BOOL=OFF^
 -DPXR_BUILD_TUTORIALS:BOOL=OFF^
 -DPXR_BUILD_USD_TOOLS:BOOL=OFF^
 -DPXR_ENABLE_PYTHON_SUPPORT:BOOL=OFF^
 -DBoost_NO_BOOST_CMAKE=NO^
 -DCMAKE_BUILD_TYPE=Debug^
 -DCMAKE_INSTALL_PREFIX:PATH=F:/usd_install/debug^
 --toolchain=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake


cmake --build . --target install -- /m:%NUMBER_OF_PROCESSORS%


```