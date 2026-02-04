copy E:\Doodle\vcpkg_ports\dnacalib\DNACalibLibExt.Target.cs "E:\UnrealEngine\Engine\Source\Programs\DNACalibLib\DNACalibLibExt.Target.cs"
copy E:\Doodle\vcpkg_ports\dnacalib\UE_DNACalib_Build.xml "E:\UnrealEngine\Engine\Source\Programs\DNACalibLib\UE_DNACalib_Build.xml"
copy E:\Doodle\vcpkg_ports\dnacalib\DNACalibLibExt.Build.cs "E:\UnrealEngine\Engine\Source\Programs\DNACalibLib\DNACalibLibExt.Build.cs"
copy E:\Doodle\vcpkg_ports\dnacalib\main.cpp "E:\UnrealEngine\Engine\Source\Programs\DNACalibLib\main.cpp"







"E:\UnrealEngine\Engine\Build\BatchFiles\RunUAT.bat" BuildGraph -Script=E:\Doodle\vcpkg_ports\dnacalib\UE_DNACalib_Build.xml -Target=DNACalibLibModule -set:Platform=Win64 -NoXGE