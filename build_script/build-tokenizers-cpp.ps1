. $PSScriptRoot\build\set_env.ps1

$vcpkg_root =  Convert-Path "$DoodleRoot\..\vcpkg"

Push-Location $DoodleRoot/external/tokenizers-cpp

# 进入 build 目录
if (-Not (Test-Path -Path "build" -ErrorAction SilentlyContinue)) {
  New-Item -ItemType Directory -Path "build" | Out-Null
}

Push-Location build
# 构建并安装
&cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install -DCMAKE_TOOLCHAIN_FILE="$vcpkg_root\scripts\buildsystems\vcpkg.cmake" -DMLC_ENABLE_SENTENCEPIECE_TOKENIZER=OFF
&cmake --build . --target install

Pop-Location
Pop-Location