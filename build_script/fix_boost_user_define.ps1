$Root = Convert-Path "$PSScriptRoot/../"

Copy-Item "$Root/vcpkg/ports/boost-config/vcpkg.json" -Destination "$Root/vcpkg_ports/boost-config/vcpkg.json"
Copy-Item "$Root/vcpkg/ports/boost-config/portfile.cmake" -Destination "$Root/vcpkg_ports/boost-config/portfile.cmake"


$RawData = Get-Content "$Root/vcpkg_ports/boost-config/portfile.cmake" -Raw

$RawData = $RawData.Replace("\n#undef BOOST_ALL_DYN_LINK\n", "\n#undef BOOST_ALL_DYN_LINK\n#define BOOST_PROCESS_USE_STD_FS\n#define BOOST_DLL_USE_STD_FS\n")

Set-Content -LiteralPath "$Root/vcpkg_ports/boost-config/portfile.cmake" `
    -Value $RawData `
    -Encoding UTF8