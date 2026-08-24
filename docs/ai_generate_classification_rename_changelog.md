# ai_generate_classification 重命名为 ai_episode 变更记录

**变更日期**: 2026-08-24  
**变更类型**: 重构/重命名  
**影响范围**: Core类型、HTTP API、数据库Schema、API文档

## 概述

将AI生成模块中的"分类（classification）"概念重命名为"剧集（episode）"，使领域术语更符合动画制作业务场景。原`ai_generate_classification`用于表示镜头序列如"sc002"、"sc003"等，重命名为`ai_episode`语义更准确。

## 详细变更

### 1. 核心类型重命名

| 原名称 | 新名称 | 文件位置 |
|--------|--------|----------|
| `ai_generate_classification` | `ai_episode` | `src/doodle_core/metadata/seedance2/` |
| `ai_generate_classification.h` | `ai_episode.h` | 头文件 |
| `ai_generate_classification.cpp` | `ai_episode.cpp` | 实现文件 |

### 2. 成员变量重命名

在`ai_generate_entity`结构体中：

| 原字段名 | 新字段名 | 说明 |
|----------|----------|------|
| `ai_generate_classification_id_` | `ai_episode_id_` | 所属剧集ID |

JSON序列化字段同步变更：
- JSON键名: `ai_generate_classification_id` → `ai_episode_id`

### 3. HTTP API变更

#### 接口类名

| 原类名 | 新类名 |
|--------|--------|
| `seedance2_subproject_ai_generate_classification` | `seedance2_subproject_ai_episode` |
| `seedance2_subproject_ai_generate_classification_instance` | `seedance2_subproject_ai_episode_instance` |

#### 路径参数

| 原参数名 | 新参数名 |
|----------|----------|
| `classification_id_` | `episode_id_` |

#### API路径变更

| 原路径 | 新路径 |
|--------|--------|
| `/api/seedance2/subproject/{subproject_id}/classification` | `/api/seedance2/subproject/{subproject_id}/episodes` |
| `/api/seedance2/subproject/{subproject_id}/classification/{classification_id}` | `/api/seedance2/subproject/{subproject_id}/episodes/{episode_id}` |
| `/api/seedance2/subproject/{subproject_id}/classification/{classification_id}/entity` | `/api/seedance2/subproject/{subproject_id}/episodes/{episode_id}/entity` |

#### 返回字段变更

删除响应中的`classification_id`字段，替换为`episode_id`。

### 4. 数据库Schema变更

| 原表名 | 新表名 |
|--------|--------|
| `seedance2_ai_generate_classification` | `seedance2_ai_episode` |

| 原列名 | 新列名 | 所属表 |
|--------|--------|--------|
| `ai_generate_classification_id` | `ai_episode_id` | `seedance2_ai_generate_entity` |

外键关联同步更新，指向新的`seedance2_ai_episode`表。

### 5. 文档变更

- 更新了`docs/seedance2.yaml` OpenAPI文档中的所有相关定义
- 标签名从"AI生成分类"改为"AI生成剧集"
- Schema从`AiGenerateClassification`改为`AiEpisode`

## 兼容性说明

⚠️ **此变更为破坏性变更**：
1. 旧API路径 `/classification` 不再可用，需使用新路径 `/episodes`
2. 旧JSON字段 `ai_generate_classification_id` 不再接受，需使用 `ai_episode_id`
3. 数据库表名变更，需执行迁移脚本（现有数据保留，通过升级逻辑自动处理）
4. 前端/客户端需同步更新API调用和字段名

## 迁移指南

### 前端/客户端调整：
1. 将所有API调用路径中的 `/classification` 替换为 `/episodes`
2. 将请求/响应中的 `classification_id` 参数替换为 `episode_id`
3. 将实体对象中的 `ai_generate_classification_id` 字段替换为 `ai_episode_id`

### 数据库迁移：
应用启动时会自动执行Schema升级，无需手动操作。升级逻辑会将旧表数据迁移到新表结构。
