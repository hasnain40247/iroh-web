export function registerIrohLanguage(monaco: any) {
  if (monaco.languages.getLanguages().some((l: any) => l.id === 'iroh')) return;

  monaco.languages.register({ id: 'iroh' });

  monaco.languages.setLanguageConfiguration('iroh', {
    brackets: [
      ['{', '}'],
      ['[', ']'],
      ['(', ')'],
    ],
    autoClosingPairs: [
      { open: '(', close: ')' },
      { open: '[', close: ']' },
      { open: '"', close: '"' },
    ],
    surroundingPairs: [
      { open: '{', close: '}' },
      { open: '[', close: ']' },
      { open: '(', close: ')' },
      { open: '"', close: '"' },
    ],
    indentationRules: {
      increaseIndentPattern: /^((?!\/\/).)*({\s*)$/,
      decreaseIndentPattern: /^\s*[}]\s*$/,
    },
  });

  monaco.languages.setMonarchTokensProvider('iroh', {
    tokenizer: {
      root: [
        [/\/\/.*$/, 'comment'],
        [/"([^"\\]|\\.)*"/, 'string'],
        [/\b\d+(\.\d+)?\b/, 'number'],
        [
          /\b(brew|craft|leaf|stem|elder|init|offer|serve|if|else|while|for|switch|case|default|break|continue|and|or)\b/,
          'keyword',
        ],
        [/\b(true|false|empty)\b/, 'constant'],
        [/[A-Z][a-zA-Z0-9_]*/, 'type'],
        [/[a-zA-Z_]\w*/, 'identifier'],
        [/[+\-*\/=!<>?:]/, 'operator'],
        [/[(){}[\],;.]/, 'delimiter'],
        [/[ \t\r\n]+/, 'white'],
      ],
    },
  });

  monaco.editor.defineTheme('iroh-dark', {
    base: 'vs-dark',
    inherit: true,
    rules: [
      { token: 'keyword',    foreground: 'c07840', fontStyle: 'bold' },
      { token: 'constant',   foreground: 'c09a40' },
      { token: 'string',     foreground: '7aaa6a' },
      { token: 'number',     foreground: 'c07850' },
      { token: 'comment',    foreground: '5e5848', fontStyle: 'italic' },
      { token: 'type',       foreground: 'c4aa50' },
      { token: 'operator',   foreground: '7898a8' },
      { token: 'delimiter',  foreground: '8a8070' },
      { token: 'identifier', foreground: 'e8dfd0' },
    ],
    colors: {
      'editor.background':                 '#333333',
      'editor.foreground':                 '#e8dfd0',
      'editor.lineHighlightBackground':    '#3a3a3a80',
      'editor.selectionBackground':        '#e0924e30',
      'editorLineNumber.foreground':       '#484848',
      'editorLineNumber.activeForeground': '#e0924e',
      'editorCursor.foreground':           '#e0924e',
      'editorIndentGuide.background':      '#3d3730',
      'editorBracketMatch.background':     '#e0924e28',
      'editorBracketMatch.border':         '#e0924e',
      'editor.findMatchBackground':        '#e0924e40',
    },
  });

  monaco.editor.defineTheme('iroh-light', {
    base: 'vs',
    inherit: true,
    rules: [
      { token: 'keyword',    foreground: '8a4420', fontStyle: 'bold' },
      { token: 'constant',   foreground: '7a6010' },
      { token: 'string',     foreground: '2a6820' },
      { token: 'number',     foreground: '8a4418' },
      { token: 'comment',    foreground: '8a7e70', fontStyle: 'italic' },
      { token: 'type',       foreground: '7a6010' },
      { token: 'operator',   foreground: '2a5870' },
      { token: 'delimiter',  foreground: '7a6e60' },
      { token: 'identifier', foreground: '211a10' },
    ],
    colors: {
      'editor.background':                 '#ece6d9',
      'editor.foreground':                 '#211a10',
      'editor.lineHighlightBackground':    '#e3dccf80',
      'editor.selectionBackground':        '#8a552030',
      'editorLineNumber.foreground':       '#c8bfb0',
      'editorLineNumber.activeForeground': '#8a5520',
      'editorCursor.foreground':           '#8a5520',
      'editorIndentGuide.background':      '#c8bfb0',
      'editorBracketMatch.background':     '#8a552020',
      'editorBracketMatch.border':         '#8a5520',
      'editor.findMatchBackground':        '#8a552030',
    },
  });
}
