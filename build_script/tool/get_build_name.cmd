set current_branch=
for /F "delims=" %%n in ('git describe --tags --match "v*"') do set "current_branch=%%n"

for /f "tokens=1 delims=-" %%i in ("%current_branch%") do (set current_branch=%%i)
set current_branch=%current_branch:~1%
set Doodle_Name=Doodle-%current_branch%-win64