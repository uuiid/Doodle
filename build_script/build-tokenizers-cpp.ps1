. $PSScriptRoot\build\set_env.ps1

Push-Location $DoodleRoot/external/tokenizers-cpp

# 进入 build 目录
if (-Not (Test-Path -Path "build")) {
  New-Item -ItemType Directory -Path "build" | Out-Null
}

Set-Location build
# 构建并安装
&cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install 
&cmake --build . --target install