---
description: "重构 HTTP put 处理器：将‘读取实体 → 改写 → update 整体写回’替换为 orm::update 链式（JSON 用 set_from_ref<T>，程序化用 set(c(&T::f_)=v)、条件 set、自增）配合 run_sql 执行"
name: refactor-put-set-from-ref
argument-hint: "选择要重构的 put 处理器"
agent: "agent"
---

将选中的 HTTP `put` 处理器重构为使用 `orm::update` 链式写法，统一通过 `run_sql` 执行。

核心目标是消除「读取实体 → 修改字段 → `co_await l_sql.update(l_entity)` 整体写回」。按数据来源分两种改写方式：

## 情况一：JSON 局部更新 → set_from_ref

### 原模式 A：整体写回
```cpp
auto l_entity = std::make_shared<T>(l_sql.get_by_uuid<T>(entity_id_));
l_json.get_to(*l_entity);
co_await l_sql.update(l_entity);
co_return in_handle->make_msg(nlohmann::json{} = *l_entity);
```

### 原模式 B：逐字段 contains 筛选
```cpp
auto l_entity = std::make_shared<T>(l_sql.get_by_uuid<T>(entity_id_));
if (l_json.contains("name")) l_json.at("name").get_to(l_entity->name_);
if (l_json.contains("description")) l_json.at("description").get_to(l_entity->description_);
co_await l_sql.update(l_entity);
co_return in_handle->make_msg(nlohmann::json{} = *l_entity);
```

### 目标模式
```cpp
using namespace orm;
auto l_update = update(l_sql).from<T>().set_from_ref<T>(l_json).where(
    c(&T::uuid_id_) == entity_id_
);
co_await l_sql.run_sql(l_update);
co_return in_handle->make_msg(nlohmann::json{} = l_sql.get_by_uuid<T>(entity_id_));
```

## 情况二：程序化字段更新 → 显式 set 链

### 原模式 C：改字段后整体 update
```cpp
auto l_entity = std::make_shared<T>(l_sql.get_by_uuid<T>(entity_id_));
l_entity->status_ = next_status;
if (cond) ++l_entity->retry_;
co_await l_sql.update(l_entity);
```

### 目标模式
```cpp
using namespace orm;
auto l_update = update(l_sql)
                    .from<T>()
                    .set(c(&T::status_) = next_status)
                    .where(c(&T::uuid_id_) == l_entity->uuid_id_);
if (cond) l_update.set(c(&T::retry_) = c(&T::retry_) + 1);
co_await l_sql.run_sql(l_update);
```

## 规则
- 保留 `person_.check_*` 权限检查和 `get_json` 调用，不改变参数与返回结构
- uuid 匹配固定使用 `&T::uuid_id_`（JSON 场景用 URL 的 id，程序化场景用已加载实体的 uuid）
- `set_from_ref` 按类型的 `put_property_list` 过滤 json 字段，忽略未出现的字段（等价于原模式 B 的逐字段 contains 筛选）
- 有条件才更新的列用 `if (cond) l_update.set(...)` 追加，不提前读回原值
- 自增/自减用 `c(&T::col_) = c(&T::col_) + 1`，避免先读旧值再整体写回
- 返回 body 基于重新读取结果（`get_by_uuid<T>`）构建，或保持原返回结构
- 仅修改选中的处理器，不触碰其它方法
