import { test } from '@playwright/test';
import { expect } from 'chai';

import { URL, JWT, PROJECT_ID, authHeader } from './config.js';
import { readJson, expectError } from './helpers.js';

// 覆盖 sqlite_upgrade.cpp 的 upgrade_category_subproject_t（37 -> 38 -> 39）与 ai_category.subproject_id：
//   1. 表结构：subproject_id 列存在、NOT NULL，外键指向 seedance2_subproject 且 ON DELETE CASCADE；
//   2. 新库直接建出该列，POST 时的归属只认 URL，不认请求体；
//   3. 列表 / 详情 / 更新 / 删除 / 类别下实体 全部按 subproject_id 收口，跨子项目一律 404；
//   4. 子项目被删除时类别级联删除；类别被删除时实体上的 ai_category_id 置空。
//
// 老库升级（user_version <= 38，含"清空旧类别与其实体 → add_column → rebuild_table"那几步）
// 无法在运行中的服务上复现：升级只在启动时跑一次，而 debug/sql 有外键约束的库上无法 DROP TABLE 重建。
// 那部分只做不依赖旧行的结构断言：user_version 已推进到 39，且列已是 NOT NULL。
//
// uuid 在库里是 16 字节 blob（sqlite_statement.h:112-137），debug/sql 的字符串字面量无法当 uuid 用，
// 所以证明归属时一律读 HTTP 返回值，或走 SQL 子查询，不直接拼接 uuid 字面量。
//
// 本文件自己造所需的最小数据：JWT 指向的人员（person.uuid）、其 ai_studio、项目（project）。
// 服务端只要是全新空库（--local + 新的 dbPath）就能直接跑，不依赖任何预置数据。

const DEBUG_SQL = `${URL}/api/doodle/debug/sql`;
const EXPECTED_USER_VERSION = 39;
const SEED_UUID_HEX = '69a8d093dcab48908f9dc51ef065d03b';
const SEED_PROJECT_HEX = 'c340051a45a64af1a750efefe639c75b';
const SEED_AI_STUDIO_HEX = '0d0d0d0d000040008000000000000001';
// person.role 存的是枚举名字符串（DOODLE_JSON_SERIALIZE_ENUM_STRICT），不是数字
const ROLE_ADMIN = "'admin'";

async function debugSql(request, sql, extra = {}) {
  const res = await request.post(DEBUG_SQL, { data: { sql, ...extra } });
  return readJson(res);
}

async function createSubproject(request, name) {
  const res = await request.post(`${URL}/api/seedance2/subproject`, {
    headers: authHeader,
    data: { name, project_id: PROJECT_ID },
  });
  const body = await readJson(res);
  expect(body.status, `创建子项目失败：${JSON.stringify(body.body)}`).to.equal(201);
  return body.body.id;
}

async function deleteSubproject(request, subprojectId) {
  const res = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}`, { headers: authHeader });
  return readJson(res);
}

async function createCategory(request, subprojectId, data) {
  const res = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/category`, {
    headers: authHeader,
    data,
  });
  return readJson(res);
}

async function listCategories(request, subprojectId) {
  const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/category`, {
    headers: authHeader,
  });
  return readJson(res);
}

async function getCategory(request, subprojectId, categoryId) {
  const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/category/${categoryId}`, {
    headers: authHeader,
  });
  return readJson(res);
}

async function updateCategory(request, subprojectId, categoryId, data) {
  const res = await request.put(`${URL}/api/seedance2/subproject/${subprojectId}/category/${categoryId}`, {
    headers: authHeader,
    data,
  });
  return readJson(res);
}

async function deleteCategory(request, subprojectId, categoryId) {
  const res = await request.delete(`${URL}/api/seedance2/subproject/${subprojectId}/category/${categoryId}`, {
    headers: authHeader,
  });
  return readJson(res);
}

async function listCategoryEntities(request, subprojectId, categoryId) {
  const res = await request.get(
    `${URL}/api/seedance2/subproject/${subprojectId}/category/${categoryId}/entity`,
    { headers: authHeader }
  );
  return readJson(res);
}

async function createEpisode(request, subprojectId, name) {
  const res = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/episodes`, {
    headers: authHeader,
    // ai_episode.from_json 要求 subproject_id 必填（服务端写入时会用 URL 覆盖它）
    data: { name, subproject_id: subprojectId },
  });
  const body = await readJson(res);
  expect(body.status, `创建剧集失败：${JSON.stringify(body.body)}`).to.equal(201);
  return body.body.id;
}

async function createEntity(request, subprojectId, episodeId, data) {
  const res = await request.post(`${URL}/api/seedance2/subproject/${subprojectId}/episodes/${episodeId}/entity`, {
    headers: authHeader,
    data,
  });
  return readJson(res);
}

async function getEntity(request, subprojectId, entityId) {
  const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectId}/entity/${entityId}`, {
    headers: authHeader,
  });
  return readJson(res);
}

test.describe('seedance2 类别子项目归属（category.subproject_id 升级校验）', () => {
  let subprojectA = null;
  let subprojectB = null;
  let categoryA = null;

  test.beforeAll(async ({ request }) => {
    // 1. JWT 指向的人员：鉴权在 handler 之前，任何业务请求都需要它。
    //    check_subproject_access 还要求 person.ai_studio_id 非空，所以先造 ai_studio 再回填。
    //    全部 OR IGNORE + 兜底 UPDATE，重复跑本文件（同一个库）也不会撞唯一约束。
    const seedPerson = await debugSql(
      request,
      `INSERT OR IGNORE INTO ai_studio (uuid_id, name, archived) ` +
        `VALUES (x'${SEED_AI_STUDIO_HEX}', 'test_category_upgrade_studio', 0); ` +
        `INSERT OR IGNORE INTO person (uuid, first_name, last_name, email, active, archived, role, is_bot, ai_studio_id) ` +
        `VALUES (x'${SEED_UUID_HEX}', 'test', 'category_upgrade', 'category_upgrade@doodle.local', 1, 0, ${ROLE_ADMIN}, 0, x'${SEED_AI_STUDIO_HEX}'); ` +
        `UPDATE person SET active = 1, archived = 0, role = ${ROLE_ADMIN}, ai_studio_id = x'${SEED_AI_STUDIO_HEX}' ` +
        `WHERE uuid = x'${SEED_UUID_HEX}';`
    );
    expect(seedPerson.status, `写入测试人员失败：${JSON.stringify(seedPerson.body)}`).to.equal(201);

    // 2. 子项目需要一个存在的项目父行；同样幂等。
    const seedProject = await debugSql(
      request,
      `INSERT OR IGNORE INTO project (uuid, name) ` +
        `VALUES (x'${SEED_PROJECT_HEX}', 'test_category_upgrade_project');`
    );
    expect(seedProject.status, `写入测试项目失败：${JSON.stringify(seedProject.body)}`).to.equal(201);

    subprojectA = await createSubproject(request, `test_category_subproject_a_${Date.now()}`);
    subprojectB = await createSubproject(request, `test_category_subproject_b_${Date.now()}`);
  });

  test.afterAll(async ({ request }) => {
    if (subprojectA) await deleteSubproject(request, subprojectA);
    if (subprojectB) await deleteSubproject(request, subprojectB);
  });

  test('数据库版本 — user_version 已推进到 39', async ({ request }) => {
    const res = await debugSql(request, 'PRAGMA user_version;');
    expect(res.status, `debug/sql 失败：${JSON.stringify(res.body)}`).to.equal(201);
    const version = Number(res.body.rows[0].user_version);
    expect(version, `user_version 应为 ${EXPECTED_USER_VERSION}，实际 ${version}`).to.equal(EXPECTED_USER_VERSION);
  });

  test('表结构 — subproject_id 列存在且为 NOT NULL', async ({ request }) => {
    const res = await debugSql(request, 'PRAGMA table_info(seedance2_ai_category);');
    expect(res.status).to.equal(201);
    const column = res.body.rows.find((row) => row.name === 'subproject_id');
    expect(column, 'seedance2_ai_category 缺少 subproject_id 列').to.not.be.undefined;
    expect(column.type, 'subproject_id 列类型应为 TEXT').to.equal('TEXT');
    const notNull = Number(column.notnull);
    expect(notNull, 'subproject_id 应为 NOT NULL（rebuild_table 之后）').to.equal(1);
  });

  test('表结构 — subproject_id 上有指向 seedance2_subproject 的外键', async ({ request }) => {
    const res = await debugSql(request, 'PRAGMA foreign_key_list(seedance2_ai_category);');
    expect(res.status).to.equal(201);
    const fk = res.body.rows.find((row) => row.from === 'subproject_id');
    expect(fk, 'seedance2_ai_category 缺少 subproject_id 外键').to.not.be.undefined;
    expect(fk.table, '外键父表应为 seedance2_subproject').to.equal('seedance2_subproject');
    expect(fk.to, '外键父列应为 uuid_id').to.equal('uuid_id');
    expect(fk.on_delete, '父行删除时应级联删除类别').to.equal('CASCADE');
  });

  test('POST .../category — 归属取 URL 的 subproject_id', async ({ request }) => {
    const res = await createCategory(request, subprojectA, { name: 'cat_owner_a', type: 'assets' });
    expect(res.status, JSON.stringify(res.body)).to.equal(201);
    expect(res.body.subproject_id, '响应里的 subproject_id 应为 URL 里的子项目').to.equal(subprojectA);
    categoryA = res.body.id;
  });

  test('POST .../category — 请求体伪造的 subproject_id 被 URL 覆盖', async ({ request }) => {
    const res = await createCategory(request, subprojectA, {
      name: 'cat_owner_a_forged',
      type: 'shot',
      subproject_id: subprojectB,
    });
    expect(res.status, JSON.stringify(res.body)).to.equal(201);
    expect(res.body.subproject_id, '归属不应被请求体改写').to.equal(subprojectA);

    await deleteCategory(request, subprojectA, res.body.id);
  });

  test('GET .../category — 列表只返回本子项目的类别', async ({ request }) => {
    const res = await listCategories(request, subprojectA);
    expect(res.status).to.equal(200);
    expect(res.body).to.be.an('array');
    expect(res.body.length).to.be.greaterThan(0);
    for (const item of res.body) {
      expect(item.subproject_id, `类别 ${item.name} 不属于本次子项目`).to.equal(subprojectA);
    }
    expect(res.body.map((item) => item.id)).to.include(categoryA);

    const other = await listCategories(request, subprojectB);
    expect(other.status).to.equal(200);
    expect(other.body.map((item) => item.id), '另一个子项目不应看到本子项目的类别').to.not.include(categoryA);
  });

  test('GET .../category/{category_id} — 跨子项目读取 404', async ({ request }) => {
    const own = await getCategory(request, subprojectA, categoryA);
    expect(own.status).to.equal(200);
    expect(own.body.id).to.equal(categoryA);
    expect(own.body.subproject_id).to.equal(subprojectA);

    const crossed = await getCategory(request, subprojectB, categoryA);
    expectError(crossed, 404, '类别不存在');
  });

  test('PUT .../category/{category_id} — 跨子项目更新 404 且数据不变', async ({ request }) => {
    const crossed = await updateCategory(request, subprojectB, categoryA, { name: 'hijacked_by_b' });
    expectError(crossed, 404, '类别不存在');

    const after = await getCategory(request, subprojectA, categoryA);
    expect(after.body.name, '跨子项目 PUT 不应改到数据').to.equal('cat_owner_a');
  });

  test('PUT .../category/{category_id} — 归属不随请求体改变', async ({ request }) => {
    const res = await updateCategory(request, subprojectA, categoryA, {
      name: 'cat_owner_a_renamed',
      subproject_id: subprojectB,
    });
    expect(res.status, JSON.stringify(res.body)).to.equal(200);
    expect(res.body.name).to.equal('cat_owner_a_renamed');
    expect(res.body.subproject_id, '归属应保持不变').to.equal(subprojectA);
  });

  test('GET .../category/{category_id}/entity — 跨子项目 404', async ({ request }) => {
    const own = await listCategoryEntities(request, subprojectA, categoryA);
    expect(own.status).to.equal(200);
    expect(own.body).to.be.an('array');

    const crossed = await listCategoryEntities(request, subprojectB, categoryA);
    expectError(crossed, 404, '类别不存在');
  });

  test('DELETE .../category/{category_id} — 跨子项目删除 404 且类别仍在', async ({ request }) => {
    const crossed = await deleteCategory(request, subprojectB, categoryA);
    expectError(crossed, 404, '类别不存在');

    const after = await getCategory(request, subprojectA, categoryA);
    expect(after.status, '跨子项目 DELETE 不应删掉类别').to.equal(200);
  });

  test('删除子项目 — 类别级联删除', async ({ request }) => {
    const cascadeSubproject = await createSubproject(request, `test_category_cascade_${Date.now()}`);
    try {
      const created = await createCategory(request, cascadeSubproject, { name: 'cat_cascade', type: 'assets' });
      expect(created.status, JSON.stringify(created.body)).to.equal(201);

      const removed = await deleteSubproject(request, cascadeSubproject);
      expect(removed.status, JSON.stringify(removed.body)).to.equal(200);

      // 子项目已删除，类别详情按归属校验同样应 404（父行不存在）
      const after = await getCategory(request, cascadeSubproject, created.body.id);
      expectError(after, 404, '类别不存在');

      // 直接按类别 uuid（转 32 位 hex 后与 blob 列对比）确认行已不存在
      const categoryHex = created.body.id.replace(/-/g, '').toUpperCase();
      const check = await debugSql(
        request,
        `SELECT COUNT(*) AS c FROM seedance2_ai_category WHERE uuid_id = x'${categoryHex}';`
      );
      expect(check.status, JSON.stringify(check.body)).to.equal(201);
      expect(Number(check.body.rows[0].c), '子项目删除后类别行应被级联删除').to.equal(0);
    } finally {
      await deleteSubproject(request, cascadeSubproject);
    }
  });

  test('删除类别 — 实体上的 ai_category_id 被置空', async ({ request }) => {
    const subproject = await createSubproject(request, `test_category_entity_${Date.now()}`);
    try {
      const category = await createCategory(request, subproject, { name: 'cat_entity', type: 'shot' });
      expect(category.status, JSON.stringify(category.body)).to.equal(201);
      const categoryId = category.body.id;

      const episodeId = await createEpisode(request, subproject, 'ep_category_entity');
      const entity = await createEntity(request, subproject, episodeId, {
        name: 'entity_category_link',
        project_uuid_id: PROJECT_ID,
        ai_category_id: categoryId,
      });
      expect(entity.status, JSON.stringify(entity.body)).to.equal(201);
      expect(entity.body.ai_category_id, '实体创建后应挂在类别上').to.equal(categoryId);

      const inCategory = await listCategoryEntities(request, subproject, categoryId);
      expect(inCategory.status).to.equal(200);
      expect(inCategory.body.map((item) => item.id)).to.include(entity.body.id);

      const removed = await deleteCategory(request, subproject, categoryId);
      expect(removed.status, JSON.stringify(removed.body)).to.equal(200);

      const after = await getEntity(request, subproject, entity.body.id);
      expect(after.status, '类别删除不应连带删除实体').to.equal(200);
      expect(after.body.ai_category_id, '类别删除后实体的 ai_category_id 应为空').to.be.null;

      expectError(await getCategory(request, subproject, categoryId), 404, '类别不存在');
    } finally {
      await deleteSubproject(request, subproject);
    }
  });

  test('鉴权 — 无 token 访问类别路由 401', async ({ request }) => {
    const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectA}/category`);
    expectError(await readJson(res), 401);
  });

  test('鉴权 — 篡改签名的 token 401', async ({ request }) => {
    const res = await request.get(`${URL}/api/seedance2/subproject/${subprojectA}/category`, {
      headers: { Cookie: `access_token_cookie=${JWT.slice(0, -4)}AAAA` },
    });
    expect(res.status()).to.equal(401);
  });
});
