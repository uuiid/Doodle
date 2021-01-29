$filelist = Get-ChildItem . -Filter *.sql
for ($file = 0; $file -lt $filelist.Count; $file++) {
    # Write-Output $filelist[$file].BaseName.split("-")[1]
    $test = "py sqlpp11-ddl2cpp {0} {1}_sqlOrm doodle" -f $filelist[$file].Name ,$filelist[$file].BaseName.split("-")[1]
    # Write-Output String.Format("sqlpp11-ddl2cpp {0} {1} doodle",$filelist[$file].Name,$filelist[$file].BaseName.split("-")[1])
    Write-Output $test
    Invoke-Expression $test
}