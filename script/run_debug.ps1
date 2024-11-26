$doodle_dir = "\\192.168.20.89\Doodle2\build\Ninja_release\_CPack_Packages\win64\ZIP";

$version_dir = Get-ChildItem -Path $doodle_dir -Attributes Directory | Sort-Object -Property Name -Descending   | Select-Object -First 1;
$doodle_dir += "\$( $version_dir.Name )\bin";
Write-Host $doodle_dir;
Start-Process -FilePath "Robocopy.exe" -ArgumentList $doodle_dir, "$PWD/bin", "/MIR", "/np", "/nfl" -Wait -NoNewWindow
Copy-Item -Path "\\192.168.20.89\Doodle2\build\doodle_auto_light_process.exe" -Destination "$PWD/bin"
# 覆盖正真的执行者
#Copy-Item -Path "$PWD/doodle_auto_light_process.exe" -Destination "$PWD/bin"
Start-Process -FilePath "$PWD/bin/doodle_kitsu_supplement.exe" -WorkingDirectory "$PWD/bin" -ArgumentList "--config=$PWD/debug_server.json"

$l_uuids = "66291e09-36ac-43b6-97f7-8ca421054c57",
"f980a323-a297-45dc-b14a-2dd5a947d958",
"789e7426-6061-4f82-8fad-cbdaa7185c81",
"38e130ea-bf50-4c11-8657-d4bcbb592e40",
"f5914907-1200-48d8-a018-5299ff6c6502",
"ae22fcf1-d143-42dd-afdc-ec1dc55cfc6e",
#"f42f973c-84bc-488c-809a-6f2860274acd",
#"72e2af7f-2584-4cc7-b4fe-00c0293b7ed6",
#"7e5057db-b251-4fde-aef9-42cff4cb15fa",
#"85ad4e4d-d6ad-4a6f-8d8c-b09c33a90d03",
#"c90e90e3-330d-4ad0-b61d-371da84e6cf9",
#"87eaf161-c7e2-462f-9c1b-daff77de16a8",
#"d30fac5f-97d7-4549-930d-e73604328f33",
#"4868e9ca-c7b0-4b26-a442-9be7136541f7",
#"85f3a5a8-df0f-4abf-98bf-b2fbce64894e",
#"34136195-12c2-4e8c-84d8-fcb171cf775f",
"12756d9b-dab1-40e4-a42f-8933bb2791dc",
"a9a91e1d-25e5-4d67-85bf-211b04819b74",
"779878fb-662d-4027-a926-af368c73a175",
"027724e7-5493-4a1c-8c95-a669f7eb3677";
foreach ($uuid in $l_uuids)
{
    Start-Process -FilePath "$PWD/bin/doodle_auto_light_client.exe" -WorkingDirectory "$PWD/bin" -ArgumentList "--address=127.0.0.1", "--uuid=$uuid"
}

