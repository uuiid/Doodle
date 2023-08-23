call %~dp0/set_venv.cmd

echo "generate %my_pwd%/build/html/file/index.html"
python %my_pwd%/docs/generate_directory_index_caddystyle.py %my_pwd%/build/html/file

echo "generate %my_pwd%/build/html/update.html"
python %my_pwd%/docs/generate_updata_log.py %my_pwd%/build/html/update.html
