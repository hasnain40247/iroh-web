import { useCallback, useEffect, useRef, useState } from 'react';
import Editor from '@monaco-editor/react';
import { registerIrohLanguage } from '../iroh-lang';
import { useTheme } from '../theme';

// ── example programs ─────────────────────────────────────────────────────────

const EXAMPLES: { name: string; code: string }[] = [
  {
    name: 'Hello World',
    code: `serve "Hello, World!";
`,
  },
  {
    name: 'Fibonacci',
    code: `craft fib(n) {
  if (n <= 1) { offer n; }
  offer fib(n - 1) + fib(n - 2);
}

brew i = 0;
while (i <= 10) {
  serve fib(i);
  i = i + 1;
}
`,
  },
  {
    name: 'Closures',
    code: `craft makeAdder(n) {
  offer craft(x) { offer x + n; };
}

brew addFive = makeAdder(5);
serve addFive(10);   // 15
serve addFive(3);    // 8

craft compose(f, g) {
  offer craft(x) { offer f(g(x)); };
}

brew double  = craft(n) { offer n * 2; };
brew inc     = craft(n) { offer n + 1; };
brew doubleInc = compose(double, inc);
serve doubleInc(4);  // 10
`,
  },
  {
    name: 'Classes',
    code: `leaf Rectangle {
  init(w, h) {
    stem.w = w;
    stem.h = h;
  }
  area      { offer stem.w * stem.h; }
  perimeter { offer 2 * (stem.w + stem.h); }
}

brew r = Rectangle(4, 6);
serve r.area;        // 24
serve r.perimeter;   // 20

leaf MathUtils {
  leaf square(n) { offer n * n; }
  leaf max(a, b) { offer a > b ? a : b; }
}

serve MathUtils.square(7);    // 49
serve MathUtils.max(10, 3);   // 10
`,
  },
  {
    name: 'Lists & Loops',
    code: `brew nums = [1, 2, 3, 4, 5];
brew sum = 0;
brew i = 0;
while (i < 5) {
  sum = sum + nums[i];
  i = i + 1;
}
serve sum;    // 15

// Nested list
brew matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];
serve matrix[1][2];   // 6

// Mutate in-place
nums[0] = 99;
serve nums[0];        // 99
`,
  },
  {
    name: 'Inheritance',
    code: `leaf Animal {
  init(name) { stem.name = name; }
  speak { offer stem.name + " speaks"; }
}

leaf Dog < Animal {
  init(name) { stem.name = name; }
  bark { offer stem.name + " says woof!"; }
}

brew dog = Dog("Rex");
serve dog.bark;   // Rex says woof!

leaf Circle {
  init(r) { stem.r = r; }
  area     { offer 3.14159 * stem.r * stem.r; }
  diameter { offer stem.r * 2; }
}

brew c = Circle(5);
serve c.area;      // ~78.5
serve c.diameter;  // 10
`,
  },
  {
    name: 'Switch & Ternary',
    code: `brew day = "monday";
switch (day) {
  case "monday":    serve "start of week";
  case "friday":    serve "end of week";
  default:          serve "midweek";
}

brew score = 85;
brew grade = score >= 90 ? "A"
           : score >= 80 ? "B"
           : score >= 70 ? "C"
                        : "F";
serve grade;   // B
`,
  },
];

// ── component ────────────────────────────────────────────────────────────────

type Interpreter = 'java' | 'c';

interface OutputLine { text: string; kind: 'out' | 'err' | 'meta' }

const DEFAULT_CODE = EXAMPLES[0].code;

export default function PlaygroundPage() {
  const { isDark } = useTheme();
  const [code, setCode]               = useState(DEFAULT_CODE);
  const [interp, setInterp]           = useState<Interpreter>('c');
  const [output, setOutput]           = useState<OutputLine[]>([]);
  const [running, setRunning]         = useState(false);
  const [editorReady, setEditorReady] = useState(false);
  const [outputW, setOutputW]         = useState(420);
  const [dragging, setDragging]       = useState(false);
  const outputRef    = useRef<HTMLDivElement>(null);
  const editorRef    = useRef<any>(null);
  const monacoRef    = useRef<any>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const isDragging   = useRef(false);
  const dragStart    = useRef({ x: 0, w: 0 });

  useEffect(() => {
    const onMove = (e: MouseEvent) => {
      if (!isDragging.current) return;
      const delta = dragStart.current.x - e.clientX;
      const next  = dragStart.current.w + delta;
      const max   = (containerRef.current?.offsetWidth ?? 900) - 280;
      setOutputW(Math.max(200, Math.min(max, next)));
    };
    const onUp = () => {
      if (!isDragging.current) return;
      isDragging.current = false;
      setDragging(false);
      document.body.style.cursor     = '';
      document.body.style.userSelect = '';
    };
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
    return () => {
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
  }, []);

  const onDividerDown = (e: React.MouseEvent) => {
    e.preventDefault();
    isDragging.current = true;
    setDragging(true);
    dragStart.current  = { x: e.clientX, w: outputW };
    document.body.style.cursor     = 'col-resize';
    document.body.style.userSelect = 'none';
  };

  useEffect(() => {
    if (!editorReady || !monacoRef.current) return;
    monacoRef.current.editor.setTheme(isDark ? 'iroh-dark' : 'iroh-light');
  }, [isDark, editorReady]);

  // scroll output to bottom on new content
  useEffect(() => {
    if (outputRef.current) {
      outputRef.current.scrollTop = outputRef.current.scrollHeight;
    }
  }, [output]);

  const runCode = useCallback(async () => {
    if (running || !code.trim()) return;
    setRunning(true);
    setOutput([{ text: `Running with ${interp === 'java' ? 'Java' : 'C'} interpreter…`, kind: 'meta' }]);

    try {
      const res = await fetch('/api/run', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ code, interpreter: interp }),
      });
      const data: { output: string; error: boolean } = await res.json();
      const lines = data.output.split('\n').map(line => ({
        text: line,
        kind: (data.error ? 'err' : 'out') as OutputLine['kind'],
      }));
      setOutput(lines);
    } catch (e) {
      setOutput([{ text: 'Could not reach the server. Is it running?', kind: 'err' }]);
    } finally {
      setRunning(false);
    }
  }, [code, interp, running]);

  // Cmd/Ctrl + Enter to run
  useEffect(() => {
    const handler = (e: KeyboardEvent) => {
      if ((e.metaKey || e.ctrlKey) && e.key === 'Enter') {
        e.preventDefault();
        runCode();
      }
    };
    window.addEventListener('keydown', handler);
    return () => window.removeEventListener('keydown', handler);
  }, [runCode]);

  function loadExample(name: string) {
    const ex = EXAMPLES.find(e => e.name === name);
    if (ex) {
      setCode(ex.code);
      setOutput([]);
    }
  }

  function handleEditorMount(editor: any, monaco: any) {
    editorRef.current = editor;
    monacoRef.current = monaco;
    registerIrohLanguage(monaco);
    monaco.editor.setTheme(isDark ? 'iroh-dark' : 'iroh-light');
    editor.updateOptions({ language: 'iroh' });
    setEditorReady(true);
  }

  const btnBase: React.CSSProperties = {
    padding: '5px 14px',
    borderRadius: 6,
    fontSize: 13,
    fontWeight: 500,
    cursor: 'pointer',
    border: '1px solid var(--border)',
    outline: 'none',
    transition: 'all 0.12s',
    fontFamily: 'inherit',
  };


  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100%', overflow: 'hidden' }}>

      {/* ── main split ── */}
      <div ref={containerRef} style={{ flex: 1, display: 'flex', overflow: 'hidden' }}>

        {/* editor pane */}
        <div style={{ flex: 1, overflow: 'hidden', minWidth: 0, position: 'relative' }}>
          {/* paper grain overlay */}
          <div style={{
            position: 'absolute',
            inset: 0,
            zIndex: 2,
            pointerEvents: 'none',
            backgroundImage: `url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='250' height='250'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.8' numOctaves='4' stitchTiles='stitch'/%3E%3CfeColorMatrix type='saturate' values='0'/%3E%3C/filter%3E%3Crect width='250' height='250' filter='url(%23n)' opacity='0.06'/%3E%3C/svg%3E")`,
            backgroundRepeat: 'repeat',
          }} />
          {dragging && (
            <div style={{ position: 'absolute', inset: 0, zIndex: 10 }} />
          )}

          {/* bottom controls */}
          <div style={{
            position: 'absolute',
            bottom: 16,
            left: 16,
            display: 'flex',
            alignItems: 'center',
            gap: 8,
            zIndex: 5,
            background: 'var(--surface)',
            borderRadius: 8,
            padding: '6px 8px',
            boxShadow: '0 4px 16px rgba(0,0,0,0.35)',
          }}>
            <div style={{ display: 'flex', alignItems: 'center', gap: 6 }}>
              <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="var(--text-muted)" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                <polyline points="16 18 22 12 16 6" />
                <polyline points="8 6 2 12 8 18" />
              </svg>
              <select
                value={interp}
                onChange={e => setInterp(e.target.value as Interpreter)}
                style={{
                  ...btnBase,
                  background: 'var(--surface)',
                  color: 'var(--text)',
                  padding: '9px 14px',
                  cursor: 'pointer',
                  fontSize: 14,
                }}
              >
                <option value="c">C</option>
                <option value="java">Java</option>
              </select>
            </div>
            <button
              onClick={runCode}
              disabled={running || !editorReady}
              title="Run (⌘ Enter)"
              style={{
                ...btnBase,
                background: running ? 'var(--surface)' : '#e0924e',
                color: running ? 'var(--text-muted)' : '#1a1a1a',
                borderColor: running ? 'var(--border)' : '#e0924e',
                padding: '9px 36px',
                fontSize: 20,
                fontWeight: 600,
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
              }}
            >
              {running ? <Spinner /> : '▶'}
            </button>
          </div>

          <Editor
            height="100%"
            defaultLanguage="iroh"
            value={code}
            onChange={v => setCode(v ?? '')}
            onMount={handleEditorMount}
            options={{
              fontSize: 16,
              lineHeight: 26,
              fontFamily: "'Chivo Mono', 'JetBrains Mono', monospace",
              fontLigatures: true,
              minimap: { enabled: false },
              scrollBeyondLastLine: false,
              renderLineHighlight: 'line',
              padding: { top: 12, bottom: 12 },
              tabSize: 2,
              insertSpaces: true,
              autoIndent: 'full',
              wordWrap: 'on',
              smoothScrolling: true,
              cursorBlinking: 'phase',
              cursorSmoothCaretAnimation: 'on',
            }}
          />
        </div>

        {/* draggable divider */}
        <div
          onMouseDown={onDividerDown}
          style={{
            width: 5,
            flexShrink: 0,
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            cursor: 'col-resize',
            background: 'transparent',
          }}
          onMouseEnter={e => { (e.currentTarget as HTMLDivElement).children[0].setAttribute('style', 'width:3px;height:100%;background:var(--accent-dim);transition:all 0.15s'); }}
          onMouseLeave={e => { (e.currentTarget as HTMLDivElement).children[0].setAttribute('style', 'width:1px;height:100%;background:var(--border);transition:all 0.15s'); }}
        >
          <div style={{ width: 1, height: '100%', background: 'var(--border)', transition: 'all 0.15s' }} />
        </div>

        {/* output pane */}
        <div style={{
          width: outputW,
          flexShrink: 0,
          display: 'flex',
          flexDirection: 'column',
          background: 'var(--bg)',
        }}>
          {/* output header */}
          <div style={{
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'space-between',
            padding: '6px 14px',
            flexShrink: 0,
          }}>
            <span style={{ fontSize: 11, fontWeight: 600, letterSpacing: '0.08em', textTransform: 'uppercase', color: 'var(--text-muted)' }}>
              Output
            </span>
            {output.length > 0 && (
              <button
                onClick={() => setOutput([])}
                style={{ ...btnBase, padding: '2px 8px', fontSize: 11, background: 'transparent', color: 'var(--text-muted)' }}
              >
                Clear
              </button>
            )}
          </div>

          {/* output body */}
          <div
            ref={outputRef}
            style={{
              flex: 1,
              overflowY: 'auto',
              padding: '12px 14px',
              fontFamily: "'JetBrains Mono', monospace",
              fontSize: 13,
              lineHeight: 1.7,
              whiteSpace: 'pre-wrap',
              wordBreak: 'break-word',
            }}
          >
            {output.length === 0 ? (
              <span style={{ color: 'var(--text-muted)', fontSize: 12 }}>
                Press <kbd style={{ background: 'var(--surface-2)', border: '1px solid var(--border)', borderRadius: 4, padding: '1px 5px' }}>⌘ Enter</kbd> or click <strong>Run</strong> to execute your code.
              </span>
            ) : (
              output.some(l => l.kind === 'err') ? (
                <div style={{
                  background: '#b0605520',
                  border: '1px solid #b0605560',
                  borderRadius: 4,
                  padding: '10px 14px',
                  color: 'var(--red)',
                  whiteSpace: 'pre-wrap',
                  wordBreak: 'break-word',
                }}>
                  {output.map(l => l.text).join('\n')}
                </div>
              ) : (
                output.map((line, i) => (
                  <div
                    key={i}
                    style={{
                      color: line.kind === 'meta' ? 'var(--text-muted)' : 'var(--green)',
                      fontStyle: line.kind === 'meta' ? 'italic' : 'normal',
                    }}
                  >
                    {line.text}
                  </div>
                ))
              )
            )}
          </div>

        </div>
      </div>
    </div>
  );
}

function Spinner() {
  return (
    <span style={{ display: 'inline-block', animation: 'spin 0.7s linear infinite', lineHeight: 1 }}>
      ◌
      <style>{`@keyframes spin { to { transform: rotate(360deg); } }`}</style>
    </span>
  );
}
