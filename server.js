import express from 'express';
import cors from 'cors';
import { exec } from 'child_process';
import { writeFileSync, unlinkSync, existsSync } from 'fs';
import { tmpdir } from 'os';
import { join } from 'path';
import { fileURLToPath } from 'url';
import { dirname } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// In dev: interpreters live one level up. In production (Railway): bundled inside repo.
const JAVA_CP = existsSync(join(__dirname, 'interpreter'))
  ? join(__dirname, 'interpreter')
  : join(__dirname, '..', 'interpreter', 'src');

const C_BIN = existsSync(join(__dirname, 'ciroh-src', 'ciroh'))
  ? join(__dirname, 'ciroh-src', 'ciroh')
  : join(__dirname, '..', 'cIroh', 'ciroh');

const app = express();
app.use(cors({
  origin: (origin, cb) => {
    // allow any localhost origin (any port) or no origin (same-origin/curl)
    if (!origin || /^https?:\/\/(localhost|127\.0\.0\.1)(:\d+)?$/.test(origin)) {
      cb(null, true);
    } else {
      cb(new Error('CORS: not allowed'));
    }
  },
}));
app.use(express.json({ limit: '512kb' }));

app.post('/api/run', (req, res) => {
  const { code, interpreter = 'java' } = req.body ?? {};
  if (typeof code !== 'string' || code.trim().length === 0) {
    return res.status(400).json({ output: 'No code provided.', error: true });
  }

  const tmpFile = join(
    tmpdir(),
    `iroh_${Date.now()}_${Math.random().toString(36).slice(2)}.iroh`
  );

  try {
    writeFileSync(tmpFile, code, 'utf8');
  } catch {
    return res.status(500).json({ output: 'Failed to write temp file.', error: true });
  }

  const cmd =
    interpreter === 'c'
      ? `"${C_BIN}" "${tmpFile}"`
      : `java -cp "${JAVA_CP}" com.interpreter.iroh.Iroh "${tmpFile}"`;

  exec(cmd, { timeout: 8000 }, (err, stdout, stderr) => {
    try { if (existsSync(tmpFile)) unlinkSync(tmpFile); } catch { /* ignore */ }

    if (err?.killed) {
      return res.json({ output: 'Execution timed out (8 s limit).', error: true });
    }

    const out    = (stdout ?? '').trimEnd();
    const errOut = (stderr ?? '').trim();

    res.json({
      output: [out, errOut ? `[stderr]\n${errOut}` : ''].filter(Boolean).join('\n') || '(no output)',
      error: !!err,
    });
  });
});

app.get('/api/health', (_req, res) => res.json({ ok: true }));

app.listen(3001, () => {
  console.log('Iroh API server → http://localhost:3001');
});
