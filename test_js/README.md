# test_js — Doodle HTTP / WebSocket 契约测试

基于 **Playwright Test** 的接口契约测试。除个别 WebSocket 用例外，全部通过 Playwright 的
`request` fixture 直接打 HTTP 接口，**不启动浏览器**——Playwright 在这里只作为 test runner
使用，提供并行执行、HTML 报告、trace 与 `--ui` 调试界面。

## 运行

```powershell
cd E:\Doodle\test_js

# 需要先启动 Doodle 服务端（默认 http://localhost:50025）
npx playwright test                    # 跑全部
npx playwright test seedance2.js       # 跑单个文件
npx playwright test -g "创建子项目"      # 按标题过滤
npx playwright test --list             # 只列出用例，不执行
npx playwright test --ui               # 交互式调试
npx playwright show-report             # 打开上次的 HTML 报告
```

对应的 npm 脚本：`npm test`、`npm run test:list`、`npm run test:ui`、`npm run test:report`。

## 配置

`playwright.config.js` 里 `baseURL` 取自环境变量 `DOODLE_BASE_URL`，默认
`http://localhost:50025`（即 Doodle 本地服务端）。测试文件内的 URL 来自
`test/config.js` 导出的 `URL`，同样读该环境变量。

单 worker 串行执行是刻意的：这些用例会真实建/删业务数据，串行可避免相互踩踏。

## 环境变量

| 变量 | 默认值 | 作用 |
| --- | --- | --- |
| `DOODLE_BASE_URL` | `http://localhost:50025` | 被测服务端地址 |
| `AI_LIGHT_ENTITY_ID` | 内置 uuid | AI 灯光用例所需的「有灯光任务的镜头」id |
| `AI_LIGHT_FULL_FLOW` | 未设置 | 设为 `1` 才跑会调用 LLM、产生真实计费的完整链路 |
| `AI_LIGHT_IMAGE_DIR` | `<仓库>/build/image` | AI 灯光参考图目录 |
| `MULTIPART_TEST_FILE` | `E:/cache/down/sadsadasd.mp4` | multipart 上传用例的源文件 |
| `MULTIPART_TASK_ID` | 内置 uuid | multipart 用例关联的任务 id |
| `APIPY_REFERENCE_FILE` | `D:\\tets.png` | apipy_image 后端参考图源文件 |
| `SEEDANCE2_REFERENCE_FILE` | `D:\\无标题(1).jpg` | 实体参考图上传源文件 |
| `SEEDANCE2_DEPTH_FILE` | `D:\\test_files\\test_depth.mp4` | 深度估计上传源文件 |
| `ANIMATION_MODEL_PATH` | `D:\\ai_mod\\onnx-models--nvidia--Kimodo-SOMA-RP-v1.1` | 动画训练用例的模型目录 |
| `DOODLE_VIDEO_SAMPLE` | `D:/test_files/ai/xuyang_data_set/xy_00.mov` | 视频截图用例的样本视频 |
| `SOCKET_IO_CONNECTION_COUNT` | `20` | socket.io 并发连接数 |
| `DISTRIBUTED_WORKER_COUNT` | `20` | 分布式工作机并发连接数 |
| `DISTRIBUTED_WORK_URL` | 由 `DOODLE_BASE_URL` 推导 | 工作机 WebSocket 地址 |

**依赖本机文件的用例在文件缺失时会自动 `test.skip()`**，而不是硬失败——所以整套测试
可以在没有这些素材的机器上跑完，只会跳过相应条目。

## 文件与结构

| 文件 | 覆盖范围 |
| --- | --- |
| `config.js` | 共享配置：URL / JWT / authHeader / 常量 / `filePart` 等工具 |
| `helpers.js` | `readJson` / `readBuffer` / `expectError` / `waitFor` / `pngSignatureHex` |
| `seedance2.js` | seedance2 子项目 / 剧集 / 实体 / 类别 / 任务（含 transfer_station、apipy_image 两个后端） |
| `canvas.js` | seedance2 无限画布：画布 / 节点 / 连线 / 可见人员、鉴权、枚举与默认值 |
| `ai_light.js` | AI 灯光全部 URI：会话 / 参考图 / Spec / 迭代判断 / 回传 / 候选产物 / 预设库 / 鉴权 |
| `test_data_user.js` | 登录、鉴权、授权、用户上下文、todo 查询 |
| `test-loc.js` | 本地作业提交（maya 检查、替换引用、自动灯光）、视频截图、socket.io 事件 |
| `test-model_library.js` | 模型库资产树与资产的增删改查 |
| `test-attendance.js` | 自定义考勤与自定义任务条目 |
| `test-work_xlsx_task_info.js` | 工作表自定义条目与考勤查询 |
| `multipart_body_test.js` | 评论预览文件 multipart 上传与错误处理 |
| `animation_train.js` | AI 动画训练（独立端口 `:50024`） |
| `socket_io.js` | socket.io 连接与并发连接 |
| `distributed_work.js` | 分布式工作机 WebSocket 上线与回执 |

`main` 是一个**独立的 socket.io 测试服务端**（监听 50025），用于验证 socket.io 客户端，
与 Doodle 的 C++ 服务端无关；它不是 Playwright 测试文件。

## 关于原始 mocha 版本的已知缺陷

从 mocha + superagent + chai 迁移过来时，顺手修掉了若干原本就必挂 / 引用缺失的问题。
如果后续对比历史版本，这些差异是刻意的：

1. `JWT` 被使用但未 import（`test_data_user.js`、`test-model_library.js`、
   `test-work_xlsx_task_info.js`）——原样运行必然 `ReferenceError`。
2. `test-attendance.js` 的「自定义任务条目」断言的时间与请求体里发的时间不一致
   （断言 `2025-03-05`，实发 `2025-04-10`），已改为断言实发值。
3. `test-work_xlsx_task_info.js` 的「添加自定义条目」先断言 `req.body` 是数组，
   又去读 `req.body.start_time`（数组上不存在该属性），已改为读 `req.body[0]`。
4. `test-loc.js` 的「获取视频截图 end」原本依赖 mocha 的 done 回调，
   已改写为直接 await 请求；「连接事件测试」把 `afterEach` 写在 `it` 内部的用法
   已改为 `try/finally` 关闭 socket。
5. 原先硬编码的本机绝对路径改为环境变量可覆盖，并在文件缺失时跳过。

## 与 C++ 单元测试的关系

本目录是**集成测试**，需要真实运行的服务端，与 `build/Ninja_debug/bin/test_main.exe`
那套 C++ 单元测试（`src/test/core/`，覆盖数据库 / 视频图片解码）互不替代：
后者不需要服务端，前者验证 HTTP 契约。
