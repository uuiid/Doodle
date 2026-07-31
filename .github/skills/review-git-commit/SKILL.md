---
name: review-git-commit
description: 'Review git commits for code quality, security, and project conventions. Use when reviewing staged/unstaged changes, a specific commit, a PR branch, or a range of commits. Keywords: code review, review commit, review diff, PR review, 审查, 代码审查, git diff.'
argument-hint: 'Commit SHA, branch, or range (e.g. HEAD~3..HEAD, main..feature/xxx). Omit to review unstaged + staged changes.'
user-invocable: true
disable-model-invocation: false
---

# Git Commit Code Review

## Outcome
Produce a structured, actionable code review for one or more git commits, focusing on correctness, safety, project conventions, and maintainability.

## When to Use
- Reviewing staged/unstaged changes before committing
- Reviewing a specific commit or range of commits
- Pre-merge PR review
- Post-commit audit for regressions

## Inputs
- **Commit range** (optional): a SHA, branch name, or range like `HEAD~3..HEAD`. If omitted, review all unstaged + staged changes (`git diff HEAD`).

## Hard Constraints
- Focus on the actual diff — do not review unchanged code.
- Flag security issues (OWASP Top 10, injection, unsafe pointer/type casts) as blocking.
- For this project: verify C++ Core Guidelines compliance, RAII, and `doodle_core`/`doodle_lib`/`exe_*` layering.
- Output must be in Chinese (zh-cn) with structured sections.

## Procedure

### 1. Collect the Diff
```bash
git diff --stat        # overview
git diff               # full diff (unstaged + staged vs HEAD)
git diff <range>       # for a specific range
```
Also run `git log --oneline -5` for context of recent commits.

### 2. Contextual Analysis
Before diving into code, identify:
- Which modules are affected? (`doodle_core`, `doodle_lib`, `maya_plug*`, `exe_*`, build scripts, CMake)
- Is this a new feature, bugfix, refactor, or dependency change?
- Does the change touch vcpkg.json, CMakeLists.txt, or Find modules?

### 3. Review Checklist

#### 3.1 Commit Message
- Is the message concise and descriptive?
- Does it explain *why*, not just *what*?
- For this project: Chinese or English is acceptable, but be consistent.

#### 3.2 Correctness
- Logic errors, off-by-one, null dereference, use-after-free.
- Exception safety: are resources released on exception paths?
- Thread safety: shared state protected? Correct strand/executor usage for Asio?
- Boost.Asio callbacks: verify `shared_from_this()` in async callbacks, not raw `this`.

#### 3.3 Ownership & Lifetime
- RAII: raw `new`/`delete` should be replaced with `std::unique_ptr`/`std::make_unique`.
- No dangling references/pointers to temporaries.
- `std::move` semantics are correct.

#### 3.4 Security (Blocking)
- Unsafe C casts (`(int)x`) → prefer `static_cast`.
- Buffer overflows, unchecked array access.
- SQL injection (if raw SQL strings are constructed).
- Command injection via `system()` or `popen()`.
- Path traversal in file operations.

#### 3.5 Project Conventions
- Layer discipline: `doodle_core` → no dependency on `doodle_lib` or `exe_*`.
- CMake: conditional Maya/FBX branches preserved, no hardcoded paths.
- `vcpkg.json` changes: overlay ports/triplets consistent.
- Error handling: exceptions (preferred) or error codes, be consistent with surrounding code.

#### 3.6 Code Quality
- Meaningful names (variables, functions, types).
- Functions: short, single responsibility.
- No dead code, commented-out blocks, debug prints.
- No magic numbers; use named constants.

#### 3.7 Testing Considerations
- Does the change need new tests? (new feature → yes; bugfix → characterization test)
- Are existing tests likely affected? Suggest running `test_main.exe`.

### 4. Output Format

Structure the review as follows:

```
## 审查摘要
- 审查范围: <range or "未暂存+已暂存">
- 影响模块: <list>
- 变更类型: <feature/bugfix/refactor/dependency>
- 总体评估: ✅ 通过 / ⚠️ 修改后通过 / ❌ 需要重做

## 严重问题 (必须修复)
- [ ] <file>: <issue> — <suggestion>

## 建议改进 (应该修复)
- [ ] <file>: <issue> — <suggestion>

## 风格/惯例 (建议修复)
- [ ] <file>: <issue> — <suggestion>

## 正面发现
- ✅ <positive observation>

## 测试建议
- 运行 `test_main.exe` (或指定测试套件)
```

### 5. Post-Review
- If the user asks to fix issues, implement changes one at a time.
- After fixes, suggest re-running the review.

## References
- [copilot-instructions.md](../../copilot-instructions.md) — project build/layer conventions
- [debugging.md](../../../memories/debugging.md) — known pitfalls (Asio lifetime, SQLite, FFmpeg)