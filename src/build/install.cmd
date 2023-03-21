call %~dp0/set_venv.cmd

call %~dp0/build_ue4_files.cmd
call %~dp0/build_houdini.cmd
call %~dp0/build_maya_plug.cmd 2018
call %~dp0/build_maya_plug.cmd 2019
call %~dp0/build_maya_plug.cmd 2020
call %~dp0/build_exe.cmd

echo -----------------copy file--------------------
if not exist "%my_pwd%\build\html\file" goto create_file_dir
goto copy_file

:create_file_dir
mkdir %my_pwd%\build\html\file

:copy_file
echo "%my_pwd%\build\html\file *.msi -> %my_pwd%\build\install\%doodle_install_prefix%"
robocopy %my_pwd%\build\install\%doodle_install_prefix% %my_pwd%\build\html\file *.msi

echo "%my_pwd%\build\html\file *.7z -> %my_pwd%\build\install\%doodle_install_prefix% "
robocopy %my_pwd%\build\install\%doodle_install_prefix% %my_pwd%\build\html\file *.7z

@REM robocopy %my_pwd%\build\install\%doodle_install_prefix% \\192.168.10.250\public\Prism_projects\doodle\ *.msi
@REM robocopy %my_pwd%\build\install\%doodle_install_prefix% \\192.168.10.250\public\Prism_projects\doodle\ *.7z

echo "generate %my_pwd%/build/html/file/index.html"
py %my_pwd%/docs/generate_directory_index_caddystyle.py %my_pwd%/build/html/file

echo "generate %my_pwd%/build/html/update.html"
py %my_pwd%/docs/generate_updata_log.py %my_pwd%/build/html/update.html
