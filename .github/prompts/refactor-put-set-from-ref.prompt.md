---
description: "重构 HTTP PUT 处理器：将‘读取实体 → JSON 写回 → update’替换为 update(...).from<T>().set_from_ref<T>(json) 通过 run_sql 执行，并用 get_by_uuid 重建返回"
name: refactor-put-set-from-ref
argument-hint: "选择要重构的 put 处理器"
agent: "agent"
---

将选中的 HTTP `put` 处理器重构为使用 `orm::update` 的 `set_from_ref` 方法。

## 原模式
```cpp
auto l_entity = std::make_shared<T>(l_sql.get_by_uuid<T>(entity_id_));
l_json.get_to(*l_entity);
co_await l_sql.update(l_entity);
co_return in_handle->make_msg(nlohmann::json{} = *l_entity);
```

## 目标模式
```cpp
using namespace orm;
auto l_update = update(l_sql).from<T>().set_from_ref<T>(l_json).where(
    c(&T::uuid_id_) == entity_id_
);
co_await l_sql.run_sql(l_update);
co_return in_handle->make_msg(nlohmann::json{} = l_sql.get_by_uuid<T>(entity_id_));
```

## 规则
- 保留 `person_.check_*` 权限检查和 `get_json` 调用，不改变参数与返回结构
- `T` 使用处理器对应的实体类型；uuid 匹配固定使用 `&T::uuid_id_`
- `set_from_ref` 按类型的 `put_property_list` 过滤 json 字段，忽略未出现的字段
- 返回 body 基于重构后的重新读取结果（`get_by_uuid<T>`）构建
- 仅修改选中的处理器，不触碰其它方法
