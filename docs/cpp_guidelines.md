# Doodle C++ 编码规范

本库以 [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)（下称 **CG**）
作为编程的**基本指导**。但它是长篇参考手册，不是可以逐条卡的门禁 —— 30 万行存量代码里，
有些规则本项目**明确不采纳**（下面 §2 逐条列出理由），其余是「新代码尽量遵守、老代码碰到再改」。

本文分三层，**写作与改代码前先看这一行**：

| 层级 | 含义 | 违反后果 |
|---|---|---|
| **硬规则** | 必须遵守，评审会打回 | 不允许；有充分理由要写进本文 §3 并说明 |
| **采纳** | 新代码遵循，老代码逐步收敛 | 可以，但别在同一处代码里两种风格混着写 |
| **豁免** | 本项目**有意**不遵守 CG 对应条目 | 不适用 —— 不是「暂时没改」，是「不打算改」 |

技术底座（2026-10 核实）：C++20（`CMAKE_CXX_STANDARD 20` + `REQUIRED ON`）、
MSVC 19.44.35228（VS 2022，`14.44.35207` 工具集）、Ninja + `x64-windows` triple、
71 项 vcpkg 依赖（Boost / OpenCV / ffmpeg / sqlite3 / nlohmann-json / spdlog / magic-enum / …）。
格式化由仓库根的 [.clang-format](../.clang-format) 固定（`BasedOnStyle: Google`、`ColumnLimit: 120`、
`AlignConsecutiveAssignments: AcrossEmptyLinesAndComments`）—— **格式问题交给工具，不进本文**。

---

## 1. 硬规则

### 1.1 F.17：不要用非常量引用/指针做「出参」，用返回值

> CG **F.17**：*For "out" output values, prefer return values to output parameters.*

```cpp
// ✗ 禁止
void make_plan(Input in, std::vector<Step>& out_steps, bool& out_ok);

// ✓ 正确
struct make_plan_result { std::vector<Step> steps_{}; bool ok_{}; };
make_plan_result make_plan(Input in);
```

即使包一层结构体显得啰嗦，也必须用返回值。这条是**用户明确要求的强制规范**。

**协程场景的注意点**：`co_await` 要求返回类型是能承载它的 awaitable，
所以**不能**直接 `co_return` 一个裸容器：

```cpp
// ✗ 编译失败：error C2039: 'promise_type': is not a member of
//   'std::coroutine_traits<std::vector<Step>, ...>'
boost::asio::awaitable<std::vector<Step>> f();

// ✓ 包一层结构体
struct spec_branch_result { orm::sql_modify_statement_vector_t sqls_{}; };
boost::asio::awaitable<spec_branch_result> advance_session_spec_branch(...);
```

**一个正面的副作用**：改用返回值后，「抛错时不能留下半截结果」这条约束从**靠约定**变成**靠类型** ——
中途抛错时返回值对象压根没被构造出来；而出参形态下它其实已被部分填过，只是双方约定不去读。
（实例见 [ue_ai_lighting_plan.md](ue_ai_lighting_plan.md) §15.21。）

`in_`/`io_` 前缀用在**真正会被修改**的入参上时，那正是本条禁止的形态：
要么改成返回值，要么把「读」与「写」拆成两次调用。

### 1.2 失败原因交给异常；只有「没有对话方」的失败才落库

- **同步请求失败**（HTTP 路由内）：异常向上抛，由 HTTP 层把 `what()` 放进响应体
  （[http_session_data.cpp](../src/doodle_lib/core/http/http_session_data.cpp) 的 `make_error_code_msg`）。
  前端**无论状态码是多少都读 body** 显示。这类失败**不要**再往业务表写一列原因 —— 那是重复，
  而且会和「异步失败」抢同一个字段。
- **异步失败**（工作机回传结果之类，人看不到那条响应体）**必须**落库，
  否则会话页会出现「无限等待中」而没有任何解释。

不要把异常吞掉转成 `bool`/错误码 —— CG **E.2**（抛异常表示「做不到」）、
**E.3**（只在异常情形用异常）在本项目就是字面执行。

### 1.3 错误必须带 HTTP 状态码

用 `DOODLE_CHICK_HTTP(cond, status, fmt, ...)`（[exception.h](../src/doodle_core/exception/exception.h)）
而不是裸 `throw`。`doodle_error::code_status_()` 从异常里取出状态码，
HTTP 层据此设响应码 —— 缺了它一律落成 500，前端拿到的「用户输入不合法」会伪装成「服务器炸了」。

### 1.4 契约字段增删要同步改测试

载荷/JSON 契约的键名清单由测试冻结（`*_json_keys_are_frozen`）。
**加键是安全的**（老解析方忽略未知键），**改名或删键**必须同时改测试与本文档对应的契约表。

---

## 2. 明确豁免的 CG 条目

以下条目本项目**有意不采纳**。这不是「暂时没改」，是评估过代价后的决定 ——
看到代码违反它们**不要**顺手"修正"，那会造成大面积无意义 diff。

### 2.1 NL.10 —— 标识符命名

CG 建议避免依赖大小写、不用下划线开头等。本项目使用既有的、全仓一致的约定：

| 前缀/后缀 | 含义 |
|---|---|
| `l_` | local —— 函数内局部量 |
| `in_` | input —— 入参 |
| `io_` | 读写入参（但见 §1.1：**不要**用它做出参） |
| `out_` | 出参（**已禁止**，见 §1.1） |
| `_`（尾随） | 类成员，如 `state_`、`uuid_id_` |

全仓一致性 > 与 CG 的字面一致。改名的代价远大于收益。

### 2.2 ES.107 —— 避免无符号下标

CG 建议用 `gsl::index` 之类避免有符号/无符号混用。本项目大量使用 `std::size_t` 索引，
因为它与 STL / `std::vector` 接口天然契合；为了避开 `ES.107` 而引入自定义索引类型，
会让每一处 `for (std::size_t i = 0; ...)` 都多一层转换。

### 2.3 F.6 / F.16 —— 优先 `string_view` / `span` 传参

项目使用 `const std::string&` / `const FSys::path&`。这与 MSVC + Windows 宽字符路径的现实相符：
`std::filesystem::path` 在 Windows 下是 `wstring`，改成 `string_view` 会在每个调用点引入编码转换。
（`FSys` = `namespace doodle::FSys { using namespace std::filesystem; }`，
见 [doodle_core_fwd.h](../src/doodle_core/doodle_core_fwd.h)。）

### 2.4 不引入 GSL

CG 多处假设你能用 `gsl::span` / `gsl::not_null` / `Expects`。本项目不引入 GSL ——
在 MSVC + vcpkg 环境下，收益低于维护成本。`Expects`/`Ensures` 的意图用 `DOODLE_CHICK` 表达。

### 2.5 不设 CI 门禁

30 万行存量代码，逐条卡规则会退化成「为了过检查而写代码」。
规则靠评审与本文档，不靠自动化。

---

## 3. 采纳（新代码遵循，老代码碰到再改）

### 3.1 资源与所有权（CG R.1 / R.3 / R.11 / C.30）

RAII、智能指针、容器 —— 项目已经在这么做：`std::shared_ptr` / `std::unique_ptr` /
`std::filesystem::path` / `std::optional` / `std::vector`。不要新增裸 `new`/`delete`。

### 3.2 类型安全（CG Enum.1 / Enum.2 / Enum.3 / I.4）

- `enum class`，不用裸 `enum`；枚举名/值经 `magic_enum` 转换而不是手写 switch。
- 强类型 id（`uuid`）而不是裸整数。
- 注意：**枚举名可能进 URL 或 JSON 契约**（例如 `ai_light_candidate_file_role`），
  那时改名就等于改接口 —— 改动前先搜测试与前端。

### 3.3 异常安全（CG E.6 / E.12）

- 抛异常的类型要能安全复制（`doodle_error` 继承 `std::runtime_error`，满足）。
- 析构函数不抛。
- **协程内注意**：C++ 标准禁止在 catch 处理块里 `co_await`。需要「记日志后再抛」时，
  先收集到局部量，出了 catch 再处理。

### 3.4 命名空间（CG SF.2 / NL.2）

按层分：`doodle::core` / `doodle::http` / `doodle::ai_light` / … 不 `using namespace` 到全局。

**实测踩过的坑**：在 `namespace doodle::http::auto_task` 里写裸名 `ai_light`，
解析到的是**兄弟**命名空间 `doodle::http::ai_light`，不是 `doodle::ai_light`。
在 `doodle::http::*` 里引用 core 的 `ai_light` 必须写全 `::doodle::ai_light::`。

### 3.5 布尔条件（CG ES.87）

不要写 `if (p != nullptr)` 这种冗余比较，写 `if (p)`。

---

## 4. 本项目特有的坑（不在 CG 里，但反复踩）

这几条是实际调试出来的，写在这里省得每个人都踩一遍。

### 4.1 `nlohmann::json{x}` 永远得到**数组**

花括号优先选 `initializer_list` 构造，**构造时就已经是数组**（`parse` 也救不回来）。对它
`erase("key")` 会抛 `[json.exception.type_error.307] cannot use erase() with array`。

要对象必须写**拷贝初始化**：

```cpp
nlohmann::json l_j = l_payload;   // ✓ 对象
nlohmann::json l_j{l_payload};    // ✗ 数组
```

同一个坑记录在 [common.h](../src/doodle_core/metadata/ai_light/common.h)。

### 4.2 `FSys::path` 的 JSON 形态是**反斜杠**

全仓**没有**启用 `adl_serializer<std::filesystem::path>` ——
[json_warp.h](../src/doodle_core/lib_warp/json_warp.h) 里那份是**注释**。
所以 `FSys::path` 走 nlohmann 内建转换，Windows 下落地为反斜杠字符串。

断言时用 `.string()` 而不是 `.generic_string()`；后者是正斜杠，**必然不等** ——
那是写法问题，不是往返丢信息。

### 4.3 `nlohmann::json` 与 `std::filesystem::path` 直接比会二义

`BOOST_TEST(l_j.at("k") == l_path)` 报 **C2666**（nlohmann 的与 `std::filesystem` 的
`operator==` 都是候选）。先取字符串：`l_j.at("k").get<std::string>() == l_path.string()`。

### 4.4 `BOOST_TEST` 要求失败时能打印双方

自定义类型没有 `operator<<` 时，`BOOST_TEST(a == b)` 触发
`error C2338: Type has to implement operator<< to be printable` —— **编译期**就失败。
逐字段比，或补一个 `operator<<`。

### 4.5 `DOODLE_CHICK` 支持 fmt 参数

`DOODLE_CHICK(cond, "值 {} 不合法", l_v)` 是合法的（`doodle_error` 走 fmt 格式化），
不必先 `fmt::format` 再传。

### 4.6 Windows 下 `path::native()` 是 `std::wstring`

`l_p.native().starts_with("..")` 不编译（C2665），要用宽字面量 `L".."`。

---

## 5. 本文档怎么改

§1 硬规则要**加**一条：说明触发的具体问题，并给出正反例。
§2 豁免要**加**一条：说明尝试过什么、代价是什么 —— 只写「不采纳」没有价值，
下一个人还会重新提议一遍。
§3/§4 直接补。

CG 本身是「参考手册」性质的，条目标号（F.17、NL.10…）可能随版本调整 ——
引用时**同时写上条款的含义**，不要只写编号。
