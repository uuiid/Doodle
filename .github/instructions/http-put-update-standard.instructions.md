---
description: "Use when writing or refactoring HTTP put handlers (HTTP 处理器, put 方法, orm update, seedance2). 统一规范：更新资源必须使用 update(...).from<T>().set_from_ref<T>(json) 配合 run_sql，禁止‘读取实体 → 写回 → update’与逐字段 contains 手工筛选。"
applyTo: "src/doodle_lib/http_method/**"
---
# HTTP put 处理器更新规范

修改或新增 `put` 处理器时，资源更新必须统一使用 `orm::update` 的 `set_from_ref` 模式。

## 统一模式

```cpp
using namespace orm;
auto l_update = update(l_sql).from<T>().set_from_ref<T>(l_json).where(
    c(&T::uuid_id_) == entity_id_
);
co_await l_sql.run_sql(l_update);
co_return in_handle->make_msg(nlohmann::json{} = l_sql.get_by_uuid<T>(entity_id_));
```

## 禁止模式

- 不先 `get_by_uuid<T>` 读实体，再 `l_json.get_to(*l_entity)` 整体写回后 `update`
- 不逐字段 `if (l_json.contains("x")) l_json.at("x").get_to(l_entity->x_)` 手工筛选

`set_from_ref` 按类型的 `put_property_list` 过滤 json 字段，忽略未出现的字段，等价于逐字段 `contains` 筛选。

- `T` 使用处理器对应的实体类型；uuid 匹配固定使用 `&T::uuid_id_`
- 保留 `person_.check_*` 权限检查与 `get_json` 调用
- 返回 body 基于重新读取结果（`get_by_uuid<T>`）构建
