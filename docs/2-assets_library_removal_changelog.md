# 资产库模块删除变更记录

**变更日期**: 2026-08-24
**变更类型**: 重构/删除
**影响范围**: Core类型、HTTP API、数据库Schema、API文档

## 概述

删除 Seedance2 模块中的资产库相关功能，包括资产分组（`assets_group`）、资产实体（`assets_entity`）与资产实体项（`assets_entity_item`）三个核心类型及其对应的 HTTP 接口、数据库表注册与 API 文档。

## 详细变更

### 1. 删除的核心类型

| 类型 | 文件位置 |
|------|----------|
| `assets_group` | `src/doodle_core/metadata/seedance2/group.h` |
| `assets_entity` | `src/doodle_core/metadata/seedance2/assets_entity.h` |
| `assets_entity_item` | `src/doodle_core/metadata/seedance2/assets_entity_item.h` |

### 2. 删除的 HTTP 实现

| 文件 | 说明 |
|------|------|
| `src/doodle_lib/http_method/seedance2/group.cpp` | 资产分组接口实现 |
| `src/doodle_lib/http_method/seedance2/assets_entity.cpp` | 资产实体接口实现 |
| `src/doodle_lib/http_method/seedance2/assets_entity_item.cpp` | 资产实体项接口实现 |

### 3. 删除的 HTTP API

以下 `/api/seedance2/asset-library/*` 接口全部删除：

| 方法 | 路径 |
|------|------|
| POST | `/api/seedance2/asset-library/group` |
| GET | `/api/seedance2/asset-library/group` |
| GET/PUT/DELETE | `/api/seedance2/asset-library/group/{group_id}` |
| GET/POST | `/api/seedance2/asset-library/group/{group_id}/entity` |
| GET/PUT/DELETE | `/api/seedance2/asset-library/entity/{entity_id}` |
| GET | `/api/seedance2/asset-library/entity/search` |
| POST | `/api/seedance2/asset-library/entity/{parent_id}/item` |
| DELETE | `/api/seedance2/asset-library/entity/{parent_id}/item/{id}` |
| GET | `/api/seedance2/asset-library/entity/{parent_id}/pictures/item/{id}.png` |
| GET | `/api/seedance2/asset-library/entity/{parent_id}/pictures/item/{id}.mp4` |
| GET | `/api/seedance2/asset-library/entity/{parent_id}/thumbnail/item/{id}.png` |

对应路由声明（`reg.h`）与路由注册（`kitsu.cpp`）均已移除。

### 4. 删除的数据库表

| 表名 | 说明 |
|------|------|
| `seedance2_assets_entity_item` | 资产实体项 |
| `seedance2_assets_entity` | 资产实体 |
| `seedance2_assets_group` | 资产分组 |

三张表之间的外键关联（`parent_id`、`group_id`、`preview_id`）同步移除。

### 5. 删除的辅助函数

| 函数 | 文件位置 |
|------|----------|
| `get_sd2_asset_library_entity_pictures_item_file` | `src/doodle_core/metadata/kitsu_ctx_t.h` |
| `get_sd2_asset_library_entity_thumbnail_item_file` | `src/doodle_core/metadata/kitsu_ctx_t.h` |

### 6. 保留项

`seedance2_animation_waiting` 端点（`/api/seedance2/animation/waiting.mp4`）与资产库无关，予以保留，其实现从 `assets_entity_item.cpp` 迁移至 `task.cpp`。

### 7. 文档变更

- 更新 `docs/seedance2.yaml` OpenAPI 文档，移除资产库相关路径定义
- 移除 Schema：`AssetsGroup`、`AssetsEntity`、`AssetsEntityItem`、`AssetsEntityWithItems`
- 移除 `资产库` tag，并同步更新文档描述

## 兼容性说明

⚠️ **此变更为破坏性变更**：

1. 所有 `/api/seedance2/asset-library/*` 接口不再可用
2. 数据库表 `seedance2_assets_group`、`seedance2_assets_entity`、`seedance2_assets_entity_item` 不再注册，历史数据不再读取
3. 前端/客户端调用资产库接口的代码需同步删除或停用

## 迁移指南

### 前端/客户端调整：

1. 移除所有资产库相关 API 调用代码
2. 移除 `assets_group`、`assets_entity`、`assets_entity_item` 相关的数据类型定义

### 数据库：

本次变更仅移除表注册，不包含历史数据清理逻辑。若需彻底清除，可手动删除 `seedance2_assets_group`、`seedance2_assets_entity`、`seedance2_assets_entity_item` 三张表。
