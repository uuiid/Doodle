import net from 'node:net';
import fs from 'node:fs';
import path from 'node:path';

const ROOT = 'E:\\Doodle\\build\\root';
const LOG = path.join(ROOT, 'logs');
if (fs.existsSync(LOG)) {
  const files = fs.readdirSync(LOG).map((f) => path.join(LOG, f)).filter((f) => fs.statSync(f).isFile());
  files.sort((a, b) => fs.statSync(b).mtimeMs - fs.statSync(a).mtimeMs);
  for (const f of files.slice(0, 3)) {
    console.log('===', f, 'mtime', fs.statSync(f).mtime.toLocaleTimeString());
    const lines = fs.readFileSync(f, 'utf8').split(/\r?\n/).filter((l) => /face_capture|摄像头|Codec|outlet|ws /.test(l));
    console.log(lines.slice(-25).join('\n'));
  }
} else {
  console.log('没有 logs 目录，列 E:\\Doodle\\build\\root:', fs.readdirSync(ROOT).join(', '));
}
