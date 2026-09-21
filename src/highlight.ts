export interface Token { text: string; type: string }

const KEYWORDS = new Set([
  'brew','craft','leaf','stem','elder','init',
  'offer','serve','if','else','while','for',
  'switch','case','default','break','continue','and','or',
]);
const CONSTANTS = new Set(['true','false','empty']);

const PATTERNS: [RegExp, string][] = [
  [/^(\/\/.*)/, 'comment'],
  [/^("(?:[^"\\]|\\.)*")/, 'string'],
  [/^(\d+(?:\.\d+)?)/, 'number'],
  [/^([A-Za-z_]\w*)/, 'word'],
  [/^([+\-*\/=!<>?:])/, 'operator'],
  [/^([\s\S])/, 'plain'],
];

export function tokenize(code: string): Token[] {
  const tokens: Token[] = [];
  let src = code;
  while (src.length > 0) {
    for (const [re, rawType] of PATTERNS) {
      const m = src.match(re);
      if (m) {
        let type = rawType;
        if (rawType === 'word') {
          if (KEYWORDS.has(m[1])) type = 'keyword';
          else if (CONSTANTS.has(m[1])) type = 'constant';
          else if (/^[A-Z]/.test(m[1])) type = 'type';
          else type = 'identifier';
        }
        tokens.push({ text: m[1], type });
        src = src.slice(m[0].length);
        break;
      }
    }
  }
  return tokens;
}

export const TOKEN_COLOR: Record<string, string> = {
  keyword:    'var(--tok-keyword)',
  constant:   'var(--tok-constant)',
  string:     'var(--tok-string)',
  number:     'var(--tok-number)',
  comment:    'var(--tok-comment)',
  type:       'var(--tok-type)',
  operator:   'var(--tok-operator)',
  identifier: 'var(--text)',
  plain:      'var(--text)',
};
