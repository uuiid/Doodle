$logger_data = "[2024-11-21 05:36:43.902] [LQ_EP219_SC103.ma] [warning] 开始运行maya",
"[2024-11-21 05:36:43.937] [LQ_EP219_SC103.ma] [warning] 开始写入配置文件 C:/Users/Administrator/AppData/Local/Temp/Doodle/cache/maya/arg/v36451/6b5e2aed-e663-47e8-9c95-5bd428bbd5ef.json",
"[2024-11-21 05:36:45.718] [LQ_EP219_SC103.ma] [info] 警告: line 1: 插件已加载。已跳过。",
"[2024-11-21 05:36:47.058] [LQ_EP219_SC103.ma] [info] 警告: file: C:\Users\Administrator\Documents\maya\2020\zh_CN\prefs\filePathEditorRegistryPrefs.mel line 4: filePathEditor: 属性 ",
"[2024-11-21 05:36:47.058] [LQ_EP219_SC103.ma] [info] 警告: file: C:\Users\Administrator\Documents\maya\2020\zh_CN\prefs\filePathEditorRegistryPrefs.mel line 5: filePathEditor: 属性 ",
"[2024-11-21 05:36:47.058] [LQ_EP219_SC103.ma] [info] 警告: file: C:\Users\Administrator\Documents\maya\2020\zh_CN\prefs\filePathEditorRegistryPrefs.mel line 6: filePathEditor: 属性 ",
"[2024-11-21 05:36:47.058] [LQ_EP219_SC103.ma] [info] 警告: file: C:\Users\Administrator\Documents\maya\2020\zh_CN\prefs\filePathEditorRegistryPrefs.mel line 7: filePathEditor: 属性 ",
"[2024-11-21 05:36:47.370] [LQ_EP219_SC103.ma] [info] 00:00:00   452MB         | log started Thu Nov 21 05:36:47 2024",
"[2024-11-21 05:36:47.370] [LQ_EP219_SC103.ma] [info] 00:00:00   452MB         | Arnold 6.1.0.0 [95d61e2c] windows clang-10.0.1 oiio-2.2.1 osl-1.11.6 vdb-4.0.0 clm-1.1.1.118 rlm-12.4.2 optix-6.6.0 2020/11/10 04:35:45",
"[2024-11-21 05:36:47.403] [LQ_EP219_SC103.ma] [info] 00:00:00   452MB         | running on DESKTOP-J8654T6, pid=12952",
"[2024-11-21 05:36:47.403] [LQ_EP219_SC103.ma] [info] 00:00:00   452MB         |  1 x Intel(R) Core(TM) i7-14700KF (28 cores, 28 logical) with 130885MB",
"[2024-11-21 05:36:47.403] [LQ_EP219_SC103.ma] [info] 00:00:00   452MB         |  NVIDIA driver version 560.94 (Optix 60807)",
"[2024-11-21 05:36:47.403] [LQ_EP219_SC103.ma] [info] 00:00:00   452MB         |  GPU 0: NVIDIA GeForce RTX 4070 @ 2475MHz (compute 8.9) with 12281MB (620MB available) (NVLink:0)",
"[2024-11-21 05:36:47.403] [LQ_EP219_SC103.ma] [info] 00:00:00   452MB         |  Windows 8 Professional (version 6.2, build 9200)",
"[2024-11-21 05:36:47.403] [LQ_EP219_SC103.ma] [info] 00:00:00   452MB         |  soft limit for open files raised from 512 to 2048",
"[2024-11-21 05:36:47.403] [LQ_EP219_SC103.ma] [info] 00:00:00   452MB         |  ",
"[2024-11-21 05:36:47.434] [LQ_EP219_SC103.ma] [info] 00:00:00   452MB         | loading plugins from C:\Program Files\Autodesk\Arnold\maya2020\bin\..\plugins ...",
"[2024-11-21 05:36:47.434] [LQ_EP219_SC103.ma] [info] 00:00:00   452MB         |  alembic_proc.dll: alembic uses Arnold 6.1.0.0",
"[2024-11-21 05:36:47.469] [LQ_EP219_SC103.ma] [info] 00:00:00   452MB         |  cryptomatte.dll: cryptomatte uses Arnold 6.1.0.0",
"[2024-11-21 05:36:47.469] [LQ_EP219_SC103.ma] [info] 警告: line 1: filePathEdi 。",
"[2024-11-21 05:36:47.469] [LQ_EP219_SC103.ma] [info] 00:00:00   452MB         |  cryptomatte.dll: cryptomatte_filter uses Arnold 6.1.0.0";

foreach ($log in $logger_data)
{
    Write-Host $log;
    Start-Sleep -Seconds 1;
}

# ps2exe E:\Doodle\script\doodle_auto_light_process.ps1 E:\Doodle\build\doodle_auto_light_process.exe
