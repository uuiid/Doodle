if exist "%~dp0..\build\compile_commands.json" del "%~dp0..\build\compile_commands.json"
mklink /H %~dp0..\build\compile_commands.json %~dp0..\build\Ninja_RelWithDebInfo\compile_commands.json