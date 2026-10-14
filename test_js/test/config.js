import jwt_encode from 'jwt-encode';
import path from 'node:path';
import fs from 'node:fs';
import { fileURLToPath } from 'node:url';

export const URL = process.env.DOODLE_BASE_URL || 'http://localhost:50025';

export const JWT = jwt_encode({
  'sub': '69a8d093-dcab-4890-8f9d-c51ef065d03b',
}, '22T0iwSHK7qkhdI6');

export const authHeader = { 'Cookie': `access_token_cookie=${JWT}` };

export const NON_EXISTENT_ID = '00000000-0000-0000-0000-000000000000';

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
