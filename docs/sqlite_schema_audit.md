# SQLite 数据库结构审计

**审计日期**: 2026-09-20
**审计对象**: `kitsu_new.db`（升级前 `user_version = 27`，757,231,616 字节）
**目标版本**: `user_version = 28`
**相关代码**: `src/doodle_lib/sqlite_orm/sqlite_database.cpp`、`sqlite_upgrade.cpp`、`orm/storage_impl.h`

---

## 一、结论摘要

| 项目 | 升级前 (v27) | 升级后 (v28) |
|------|-------------|-------------|
| `user_version` | 27 | 28 |
| 业务表 | 76 | 73 |
| 索引 | 262 | 225 |
| 纯冗余索引 | 35 | 0 |
| 外键总数 | 131 | 131（成分变了，见 4.1） |
| 外键违规行 | 12,466 | 0 |
| `_backup` 残留表 | 0 | 0 |
| `integrity_check` | ok | ok |
| `comment` 行数 | 532,578 | 532,453 |
| `task` 行数 | 260,815 | 260,718 |

清理掉的 13,116 行都是**违反外键约束的孤儿行**，不是正常数据。其中最大的一笔是
`playlist_shot` 24,586 → 14,731（-9,855）：这些是 P0 bug 期间（绝大多数连接上
`foreign_keys` 为 OFF，级联删除从未触发）留下的历史损坏，`playlist_shot.playlist_id`
是 `not_null` 且早已声明为 `CASCADE`，也就是说在约束真正生效的情况下它们本就不该存在。

---

## 二、连接级 PRAGMA 修正（P0）

**问题**：`PRAGMA foreign_keys` 是**连接级**开关，默认 OFF。而连接池里每个连接都要单独设置。
原先 `upgrade()` 只在自己拿到的那个连接上设置 pragma，于是池里其余连接全部没有外键约束——
这既让级联删除失效（上面 13,116 行孤儿的来源），也让约束校验形同虚设。

**修正**：在 ORM 注册一个连接扩展钩子，让**每一个新连接**都执行一遍 pragma：

| PRAGMA | 作用域 | 值 |
|--------|--------|-----|
| `foreign_keys` | 连接 | ON |
| `recursive_triggers` | 连接 | ON |
| `journal_mode` | 文件 | WAL |

注意 `PRAGMA foreign_keys` 在事务内是 **no-op**，必须在 `BEGIN` 之前执行。

---

## 三、外键动作约定

`ON DELETE` 写在 `CREATE TABLE` 的 DDL 里，**改 C++ 里的声明对已存在的表毫无影响**，
必须重建表才会生效（见第七节）。约定如下：

| 关系类型 | 动作 | 理由 |
|----------|------|------|
| 业务数据 → 业务数据 | `CASCADE` | 子行离开父行没有意义（如 `task.entity_id`） |
| 业务数据 → 字典数据 | `NO ACTION` | 字典项被引用时不允许删除，宁可报错也不能静默连带删除业务数据 |
| 可空的可选归属 | `SET NULL` | 归属者没了，归属关系置空（如 `task.assigner_id`） |
| 链接表 | `CASCADE` | 链接表的存在意义就是两侧都还在 |

判定为「字典数据」的表共 4 张：`task_type`、`task_status`、`asset_type`、`project_status`。

**为什么选 `NO ACTION` 而不是 `SET NULL`**：`task.task_type_id` 等列是 `not_null` 的，
置空会直接违反非空约束；而 `NO ACTION`（外键在语句末尾校验）会在有子行时**拒绝删除并明确报错**，
不会丢数据，比静默置空/静默连带删除都安全。

### 3.1 改为 `NO ACTION` 的 10 个外键

| 表 | 列 | 引用 |
|----|----|------|
| `task` | `task_type_id` | `task_type(uuid)` |
| `task` | `task_status_id` | `task_status(uuid)` |
| `entity` | `entity_type_id` | `asset_type(uuid)` |
| `playlist` | `task_type_id` | `task_type(uuid)` |
| `comment` | `task_status_id` | `task_status(uuid)` |
| `status_automation` | `in_task_type_id` | `task_type(uuid)` |
| `status_automation` | `in_task_status_id` | `task_status(uuid)` |
| `status_automation` | `out_task_type_id` | `task_type(uuid)` |
| `status_automation` | `out_task_status_id` | `task_status(uuid)` |
| `project` | `project_status_id` | `project_status(uuid)` |

### 3.2 顺带修正的 2 个 `not_null` 列

对比重建前后的外键清单时发现，另有两个外键也发生了变化：

| 表 | 列 | 老库 | 现在 | 说明 |
|----|----|------|------|------|
| `comment` | `person_id` | `SET NULL` | `NO ACTION` | 该列是 `not_null()` |
| `seedance2_subproject` | `created_user_id` | `SET NULL` | `NO ACTION` | 该列是 `not_null()` |

这不是回归而是修正：**`ON DELETE SET NULL` 作用在 `NOT NULL` 列上本来就无法生效**——
外键动作会执行一次「把该列更新为 NULL」，随即撞上非空约束，父行删除照样失败，
只是错误信息变成难以理解的 `NOT NULL constraint failed`。改成 `NO ACTION` 后
父行删除仍然被拒，但报的是明确的外键约束错误。

### 3.3 行为变化（需要知会使用方）

- 字典项被引用时**不能再删除**。此前 `DELETE` 会静默把所有引用它的业务数据一并级联删除
  （例如删一个 `task_status` 会删掉所有该状态的任务）。
- 目前唯一走这条路径的接口是 `http_method/kitsu/base_data.cpp:225` 的
  `data_entity_types_instance::delete_`，它**没有做引用检查**。现在会因外键约束失败而返回 400
  （见第六节的 HTTP 错误处理），不会再静默删数据。
- 对照的先例：`http_method/model_library/assets_tree.cpp:117` 用
  `make_error_code_msg(bad_request, "该节点有子节点无法删除")` 处理同类情形。

---

## 四、本次结构变更清单

### 4.1 新增/修正外键

**此前是裸列，本次补上约束：**

| 表 | 列 | 引用 | 动作 |
|----|----|------|------|
| `assets_tab` | `parent_uuid` | `assets_tab(uuid_id)` | `CASCADE` |
| `work_xlsx_task_info_tab` | `project_id` | `project(uuid)` | `CASCADE` |

两者在真实库中都是 **0 条孤儿**，可以直接补约束而无需清理数据。
`assets_tab.parent_uuid` 是自引用树，动作与同类的 `entity.parent_id` / `entity.source_id` 保持一致。

**此外，生产库（v27）里 `comment.object_id` 根本没有外键**——它指向 `entity(uuid)` 的旧声明
在真实数据上对全部 53 万行都不成立。重建会把它按当前声明改成 `task(uuid)` 并加上 `CASCADE`。

> 这三处是「重建会补上当前声明里有、老库里没有的外键」的体现，也解释了为什么
> 升级前后外键总数都是 131：**+3（上面三个）−3（随废弃表一起消失的
> `ai_image_metadata.author` 与 `metadata_descriptor_department_link` 的两个）**。
> 这类差异必须靠重建前后来对比外键清单才能发现，不能靠加减法推算。

### 4.2 删除的废弃表

| 表 | 真实库行数 | 说明 |
|----|-----------|------|
| `metadata_descriptor` | 0 | 代码已不再使用 |
| `metadata_descriptor_department_link` | 0 | 代码已不再使用 |
| `ai_image_metadata` | 0 | 连同其 REST 资源一起删除，见 4.3 |

这三张表已从 `regs_all()` 摘除，因此 `rebuild_all_tables` 不会再处理它们。
`rebuild_all_tables` 只遍历**已注册**的表，删不掉未注册的表，所以升级流程里必须有
`drop_obsolete_tables` 这一步显式删除（见第七节）。

### 4.3 `ai_image_metadata` 及其 REST 资源

该表虽然 0 行，但代码里仍挂着一整套对外接口，删表会让这些路径在运行时失败
（ORM 找不到表注册，抛异常 → 500 且附带完整调用栈）。因此一并删除：

| 删除对象 | 位置 |
|----------|------|
| `GET` 列表 / `POST` 创建 / `DELETE` 实例 | `http_method/model_library/ai_image.cpp`（整个文件） |
| 路由注册 `/api/doodle/ai_image`、`/api/doodle/ai_image/{id}` | `http_method/kitsu.cpp` |
| 处理类声明 | `http_method/model_library/model_library.h` |
| 生成缩略图后回写宽高 | `http_method/model_library/thumbnail.cpp` |
| 构建清单 | `http_method/model_library/CMakeLists.txt` |

**这是一处对外可见的接口删除**。判断依据是表内 0 行、且功能与 `seedance2` 那套 AI 能力重复。
类型定义 `doodle_core/metadata/ai_image_metadata.h` 本身暂时保留（属公开类型，删除另议）。

### 4.4 废弃但保留的列

| 表 | 列 | 真实库非空行数 | 处理 |
|----|----|---------------|------|
| `preview_file` | `source_file_id` | 0（整列为 NULL） | **保留声明**，仅标注废弃 |

这里必须注意：重建时列的拷贝清单来自 **ORM 声明**而非数据库实际结构，
所以一旦把 `add_column("source_file_id", ...)` 删掉，这一列就会在本次重建中被真正删除。
既然决定「后期再删」，就**必须保留声明**，只加注释说明它已废弃。

### 4.5 唯一性约束修正

| 表 | 原状 | 现状 | 理由 |
|----|------|------|------|
| `preview_file` | `name` 单列 `unique()` | 取消 | 同一个 `name` 在不同 `task`/`revision` 下本就会重复出现，单列唯一会让合法数据插不进去。唯一性由 `(name, task_id, revision)` 复合唯一索引保证 |
| `project_asset_type_link` | 无唯一索引 | `(project_id, asset_type_id)` 唯一 | 否则同一对（项目, 资产类型）可以重复插入 |
| `project_person_link` | 无唯一索引 | `(project_id, person_id)` 唯一 | 同上 |

加唯一索引前已确认真实库中**不存在重复行**，因此不会因建索引失败而中断升级。

### 4.6 未处理

| 表 | 说明 |
|----|------|
| `seedance2_canvas_element` | 开发中的表，本次忽略 |

---

## 五、冗余索引

### 5.1 什么是「纯冗余索引」

指**与某个 `sqlite_autoindex_*`（由 `UNIQUE` / `PRIMARY KEY` 自动生成）列集合完全相同**的显式索引。
它不提供任何额外的查询能力，只增加写入开销和文件体积。

真实库升级前有 **35** 个。来源有两处：

1. `table_info::add_foreign_key` 会顺带为**被引用列**建索引。但被引用列必然是父表的
   PK 或 `UNIQUE` 列，SQLite 已经为它生成了 `sqlite_autoindex_*`，再建一个是纯冗余。
   → 已从 `orm/storage_impl.h:156` 移除该行。
2. 5 张表的 `uuid` 列已经声明了 `unique()`，却又额外调用了 `.add_index(&X::uuid_id_)`：
   `seedance2::task`、`server_task_info`、`attendance_helper::database_t`、`organisation`、
   `updata_logs`。→ 已删除这 5 处显式索引。

### 5.2 两种清理途径

- **已注册的表**：重建时会按 `l_old_table->indexes_` 重新建索引，而 `DROP TABLE` 会连带删掉
  表上所有索引，所以重建天然清掉多余的索引（真实库上这一途径清掉了 31 个）。
- **未注册的遗留表**：不参与重建，需要 `drop_redundant_indexes` 显式处理
  （真实库上删掉 4 个；它只按**常量参数**调用 `pragma_index_info`，不依赖相关子查询）。

两者合计把 35 个纯冗余索引清到 0，索引总数 262 → 225。

---

## 六、HTTP 错误处理

数据库约束错误（`orm::sqlite_orm_exception`）统一映射为 **400 Bad Request**，
在 `core/http/http_session_data.cpp` 中于 `catch (...)` **之前**拦截。

原先这类异常会落到 `catch (...)`，返回 500 并附带
`boost::current_exception_diagnostic_information()`——也就是完整调用栈和内部路径，
既暴露实现细节又对使用方没有意义。

**已知取舍**：这是统一映射，因此 `SQLITE_BUSY`（数据库被锁）、`SQLITE_IOERR` 这类
**服务端故障**也会返回 400。如需区分，判断 `SQLITE_CONSTRAINT`（错误码 19）即可。
另外 400 的响应体是 `e.what()`，约束错误时会包含失败的 SQL
（形如 `787: FOREIGN KEY constraint failed (执行 DELETE FROM ...)`）。

---

## 七、升级机制

`ON DELETE` 写在 DDL 里，所以**任何外键动作的改动都必须重建表**。v27 → v28 的单步升级
（`details::upgrade_1_t`）依次做四件事：

| 步骤 | 作用 |
|------|------|
| `backup(l_s)` | 升级前把整库备份到 cache 目录的 `backup/kitsu_<时间戳>.db` |
| `rebuild_all_tables(l_s)` | 按当前 ORM 声明重建**每一个已注册的表**（`CREATE` 新表 → `INSERT ... SELECT` → `DROP` 旧表 → `RENAME`），使新的外键动作生效 |
| `fix_foreign_key_violations` | 关外键 + 事务，删除所有孤儿行；`drop_redundant_indexes` 清理未注册表上的冗余索引；`drop_obsolete_tables` 删除废弃表 |
| `vacuum()` | 回收空间（需要一份等大的临时空间） |
| `user_version(28)` | 标记完成 |

### 7.1 版本判断用 `> 27 就跳过`，不是 `== 27`

这一步的内容是此前所有升级动作的**并集**，任何更旧的库都能被它一次带到最新。
若写成 `== 27`，停在 v26 的库会既不满足条件、也不会被写入新版本号，
于是永远留在旧 schema 上——不报错，但迁移从未发生。`upgrade_init_t` 会先把新库
（`user_version == 0`）标记为当前版本，所以新建的库不会重复走这一步。

### 7.2 重建的两个必要条件

- `legacy_alter_table = ON`：否则 `ALTER TABLE ... RENAME` 会去改写其它表/触发器里
  对该表名的引用，导致 `error in trigger rb_parent_audit_trigger` 之类的失败。
- 触发器按**所属表**过滤后单独重建，否则重建过程中会因引用尚未建好的表而报
  `no such table: main.entity`。

### 7.3 注意事项

- 重建时列的拷贝清单来自 **ORM 声明**，不是数据库实际的列。因此把某列从 `regs_all()`
  里摘掉，等价于在重建时**删除该列**。反过来说，想保留一列就必须保留它的声明——
  即使该列已废弃（第 4.4 节的 `source_file_id` 正是因此保留声明）。
- `VACUUM` 需要一份与库等大的临时空间。757 MB 的库要预留约 760 MB。

---

## 八、已知遗留问题

- `preview_file.source_file_id` 仍是废弃列，只是本次不删（见 4.4）。
- `doodle_core/metadata/ai_image_metadata.h` 类型定义已无使用者，可另行删除。
- `entity_asset_extend_2.entity_id → entity` 曾有 134 条真孤儿（该列为 `NOT NULL`，
  无法置空），已由本次迁移清理。
- `task.last_preview_file_id`、`work_xlsx_task_info_tab.kitsu_task_ref_id` 在清理后仍有孤儿，
  补约束前需要再清一次。
- `project_preview_background_file_link`、`project_status_automation_link` 也没有唯一索引。

---

## 九、审计复现方法

全部为只读查询，可直接在 `sqlite3` 上跑（建议加 `-readonly`）。

```sql
-- 版本
SELECT * FROM pragma_user_version;

-- 表数 / 索引数
SELECT count(*) FROM sqlite_master WHERE type = 'table' AND name NOT LIKE 'sqlite_%';
SELECT count(*) FROM sqlite_master WHERE type = 'index';

-- 外键总数（相关子查询 + 表值函数，已验证可用）
SELECT sum(c) FROM (
  SELECT (SELECT count(*) FROM pragma_foreign_key_list(m.name)) AS c
  FROM sqlite_master m WHERE m.type = 'table' AND m.name NOT LIKE 'sqlite_%'
);

-- 纯冗余索引：显式索引的列集合与某个 sqlite_autoindex_* 完全相同
SELECT m.tbl_name, m.name FROM sqlite_master m
WHERE m.type = 'index' AND m.sql IS NOT NULL
  AND EXISTS (
    SELECT 1 FROM sqlite_master a
    WHERE a.type = 'index' AND a.sql IS NULL AND a.tbl_name = m.tbl_name
      AND (SELECT group_concat(ii.name) FROM pragma_index_info(a.name) ii)
        = (SELECT group_concat(ii.name) FROM pragma_index_info(m.name) ii)
  );

-- 外键违规（12,466 行；升级后应为 0）
PRAGMA foreign_key_check;

-- 完整性
PRAGMA integrity_check;

-- 某表的外键动作
SELECT "from", "table", "to", on_delete FROM pragma_foreign_key_list('task');
```

**注意**：`pragma_foreign_key_list` 的 `"from"` 是 SQL 关键字，必须加引号。

### 9.1 相关测试

| 测试文件 | 覆盖内容 |
|----------|----------|
| `src/test/core/sqlite_fk_semantics.cpp` | 字典表删除被拒 + 各表 `ON DELETE` 动作的声明回归 |
| `src/test/core/sqlite_schema.cpp` | 空库建表、废弃表/列不再创建、唯一索引形态 |
| `src/test/core/sqlite_upgrade.cpp` | 版本门控、新库/旧库升级路径 |
| `src/test/core/sqlite_real_db.cpp` | 真实库端到端（需 `DOODLE_REAL_DB` 指向库文件） |

`sqlite_real_db` **不会修改所指向的数据库**：每个用例都先把 `DOODLE_REAL_DB` 复制到
临时目录（连同 `-wal`/`-shm`）再在工作副本上操作，用例结束时删除工作副本。
即便如此，指向真实库时仍建议先确认磁盘上有约 760 MB 的余量（`VACUUM` 需要等大临时空间）。

```powershell
$env:DOODLE_REAL_DB = "E:\Doodle\build\kitsu_new.db"
.\build\Ninja_debug\bin\test_main.exe --run_test=sqlite_real_db
```
