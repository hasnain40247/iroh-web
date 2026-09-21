import { tokenize, TOKEN_COLOR } from '../highlight';

interface Props { code: string }

export default function CodeBlock({ code }: Props) {
  const tokens = tokenize(code);

  return (
    <pre style={{
      background: 'var(--surface-2)',
      border: '1px solid var(--border)',
      borderRadius: 0,
      padding: '13px 18px',
      overflowX: 'auto',
      fontSize: 13,
      lineHeight: 1.75,
      margin: '10px 0 20px',
      fontFamily: "'Chivo Mono', 'JetBrains Mono', monospace",
    }}>
      <code>
        {tokens.map((t, i) => (
          <span key={i} style={{ color: TOKEN_COLOR[t.type] ?? 'var(--text)' }}>
            {t.text}
          </span>
        ))}
      </code>
    </pre>
  );
}
