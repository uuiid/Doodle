import jwt_encode from 'jwt-encode';
import path from 'node:path';
import fs from 'node:fs';
import { fileURLToPath } from 'node:url';

export const URL = process.env.DOODLE_BASE_URL || 'http://localhost:50025';

export const JWT = jwt_encode({
  'sub': '69a8d093-dcab-4890-8f9d-c51ef065d03b',
}, '22T0iwSHK7qkhdI6');

export const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };

export const NIL_ID = '00000000-0000-0000-0000-000000000000';

export const NON_EXISTENT_ID = '00000000-0000-0000-0000-000000000001';

export const PROJECT_ID = 'c340051a-45a6-4af1-a750-efefe639c75b';

export const PERSON_ID = '69a8d093-dcab-4890-8f9d-c51ef065d03b';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

export const WORKSPACE_ROOT = path.resolve(__dirname, '..', '..');

export const BUILD_DIR = path.join(WORKSPACE_ROOT, 'build');

export const IMAGE_DIR = process.env.AI_LIGHT_IMAGE_DIR || path.join(BUILD_DIR, 'image');

export function listReferenceImages(dir = IMAGE_DIR) {
  if (!fs.existsSync(dir)) return [];
  return fs
    .readdirSync(dir)
    .filter((n) => /\.(png|jpg|jpeg|webp|tif|tiff|exr|bmp)$/i.test(n))
    .map((n) => path.join(dir, n))
    .sort();
}

export function filePart(filePath, name = path.basename(filePath)) {
  return {
    name,
    mimeType: mimeTypeOf(filePath),
    buffer: fs.readFileSync(filePath),
  };
}

export function mimeTypeOf(filePath) {
  const ext = path.extname(filePath).toLowerCase();
  const table = {
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.jpeg': 'image/jpeg',
    '.webp': 'image/webp',
    '.gif': 'image/gif',
    '.bmp': 'image/bmp',
    '.tif': 'image/tiff',
    '.tiff': 'image/tiff',
    '.exr': 'image/x-exr',
    '.mp4': 'video/mp4',
    '.mov': 'video/quicktime',
    '.json': 'application/json',
    '.txt': 'text/plain',
  };
  return table[ext] || 'application/octet-stream';
}

export function requireEnvFile(envName, fallback) {
  const value = process.env[envName] || fallback || null;
  if (!value) return null;
  return fs.existsSync(value) ? value : null;
}

export function nowMs() {
  return Date.now();
}

// ---------------------------------------------------------------------------
// 面部捕捉（src/doodle_lib/http_method/local/face_capture.h）
//
// 这九条路由只注册在 create_kitsu_local_route()，所以被测服务端必须带 --local 启动。
// 会话根目录 = core_set::get_cache_root()/"face_capture"/"sessions" = <root>/face_capture/sessions
// （capture_session.cpp:523-525），--local 分支会用 --root 覆盖 core_set 的 p_root，
// 所以服务端以 --root 指向下面这个目录时，会话目录是确定的。
// ---------------------------------------------------------------------------

export const FACE_CAPTURE_API = `${URL}/api/doodle/local/face_capture`;

export const FACE_CAPTURE_ROOT = process.env.DOODLE_ROOT || path.join(BUILD_DIR, 'root');

export const FACE_CAPTURE_SESSIONS_DIR =
  process.env.FACE_CAPTURE_SESSIONS_DIR || path.join(FACE_CAPTURE_ROOT, 'face_capture', 'sessions');

export const FACE_CAPTURE_SAMPLE =
  process.env.FACE_CAPTURE_SAMPLE || path.join(BUILD_DIR, 'shi_ping', 'MySlate_20', 'front-1-wide.mp4');

export const FACE_CAPTURE_SEED_DIR =
  process.env.FACE_CAPTURE_SEED_DIR || path.join(BUILD_DIR, 'shi_ping', 'MySlate_20_face_test');

export const FACE_CAPTURE_TRANSCODE_E2E = process.env.FACE_CAPTURE_TRANSCODE_E2E === '1';

export const FACE_CAPTURE_WAIT_MS = Number(process.env.FACE_CAPTURE_WAIT_MS || 20000);

export const FACE_CAPTURE_UE_PROJECT = process.env.FACE_CAPTURE_UE_PROJECT || 'E:\\ue_main\\ue_main.uproject';
