---
name: refactor-doodle-orm
description: 'Refactor database ORM code from sqlite_orm to doodle::orm namespace SQL ORM. Use when migrating sqlite_orm types, queries, schema bindings, and call sites with safety checks, compatibility decisions, and staged rollout.'
argument-hint: 'Target module or file scope, e.g. sqlite_database.cpp or src/doodle_lib/sqlite_orm/**'
user-invocable: true
disable-model-invocation: false
---

# Refactor sqlite_orm to doodle::orm

## Outcome
Produce a safe, staged migration from `sqlite_orm` usage to a first-party `doodle::orm` SQL ORM API, while preserving behavior, query correctness, and build/test stability.

## When to Use
- You want to remove dependency on `sqlite_orm` internals in production code.
- You need explicit ownership of ORM abstractions under `doodle::orm`.
- You are touching schema mapping, CRUD/query builders, or transaction boundaries.
- You need migration strategy decisions: compatibility shim vs direct rewrite.

## Inputs
- Refactor scope (single file, module, or full repo path) from the current conversation.

## Hard Constraints
- Default migration mode is direct rewrite. Do not use compatibility shim unless explicitly requested.
- Use exceptions as the unified error handling policy for `doodle::orm`.
- Migration scope must be explicitly specified in the conversation. If scope is missing or ambiguous, stop and request scope instead of migrating.
- No external interface compatibility retention is required. Legacy external APIs can be replaced directly.

## Procedure
1. Baseline and map current usage.
   - Locate all `sqlite_orm` includes, namespaces, type aliases, and query builders in scope.
   - Capture current behavior contracts: schema creation, joins, filters, ordering, null handling, transactions, exceptions/error codes.
   - Record compile/test baseline before any changes.

2. Define migration strategy.
    - Use direct rewrite as default.
    - Replace storage/schema/query construction directly with new `doodle::orm` implementation.
    - Use compatibility shim only when the conversation explicitly asks for it.
    - Document rollback approach for the scoped changes.

3. Design target API in `doodle::orm`.
   - Keep domain-level intent readable: repository/service code should not leak backend-specific details.
   - Make ownership/lifetime explicit (RAII and value semantics where possible).
   - Standardize error policy as exceptions.

4. Build a thin anti-corruption layer.
   - Encapsulate SQL/ORM backend details behind stable interfaces.
   - Add conversion helpers for entities, optionals/nullables, time types, and enums.
   - Ensure transaction and connection boundaries are explicit and testable.
   - Do not keep legacy external compatibility wrappers unless explicitly required.

5. Migrate call sites incrementally.
   - Update includes and namespace usage to `doodle::orm`.
   - Replace query expressions in small batches with behavior-equivalent forms.
   - After each batch, build and run targeted tests.

6. Validate behavior parity.
   - Verify CRUD parity and edge cases:
     - null and default values
     - filtering and ordering semantics
     - join cardinality and projection mapping
     - transaction commit/rollback behavior
   - Add characterization tests where legacy behavior is unclear.

7. Remove old dependency surface.
   - Eliminate remaining direct `sqlite_orm` usage from migrated scope.
   - Keep compatibility shim only if explicitly approved for temporary coexistence.
   - Update CMake/vcpkg usage if dependency is no longer needed.

8. Final gates.
   - Configure/build using project wrapper scripts and the appropriate CMake preset.
   - Run relevant Boost tests and migration-specific tests.
   - Produce a migration summary: changed API, known differences, and follow-up tasks.

## Decision Points
- If query semantics are hard to prove equivalent:
  - Add characterization tests first, then migrate.
- If scope is missing or not concrete:
   - Stop and ask for explicit scope; do not perform migration.
- If ABI/public API impact exists:
   - Proceed with replacement unless the conversation requests compatibility retention.

## Quality Criteria
- No direct `sqlite_orm` references remain in migrated scope (unless the conversation explicitly allows temporary shim).
- Build passes for affected presets.
- Relevant Boost tests pass and new ORM tests cover critical paths.
- Transaction, null, and join behavior match baseline expectations.
- Error handling is exception-based, consistent, and documented.

## Completion Checklist
- Baseline captured and compared.
- Target `doodle::orm` API documented in code.
- Explicit scope is recorded from the conversation.
- Direct rewrite path is recorded (or explicit exception documented).
- Incremental batches validated.
- Old surface reduced or removed.
- Final verification and summary completed.

## Suggested Prompt Examples
- `/refactor-doodle-orm 迁移范围: src/doodle_lib/sqlite_orm/sqlite_database.cpp; 直接重写为 doodle::orm; 统一异常处理; 不保留外部兼容接口。`
- `/refactor-doodle-orm 迁移范围: src/doodle_lib/sqlite_orm/**; 执行分批直接重写并在每批后运行 Boost 测试。`
- `/refactor-doodle-orm 仅做范围审计: src/doodle_lib/sqlite_orm/**; 如果范围不完整先停止并返回缺失项。`