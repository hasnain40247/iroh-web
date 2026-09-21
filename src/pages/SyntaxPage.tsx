import { useEffect, useRef, useState } from 'react';
import CodeBlock from '../components/CodeBlock';

// ── primitive doc components ──────────────────────────────────────────────────

function H2({ children }: { id?: string; children: React.ReactNode }) {
  return (
    <h2 style={{
      display: 'flex',
      alignItems: 'center',
      gap: 14,
      fontSize: 22,
      fontWeight: 600,
      letterSpacing: '-0.02em',
      marginBottom: 16,
    }}>
      <span style={{
        background: '#e0924e',
        color: '#fff',
        padding: '2px 10px',
        whiteSpace: 'nowrap',
      }}>
        {children}
      </span>
      <span style={{
        flex: 1,
        height: 1,
        background: 'var(--border)',
      }} />
    </h2>
  );
}

function H3({ children }: { id?: string; children: React.ReactNode }) {
  return (
    <h3 style={{
      fontSize: 16,
      fontWeight: 600,
      color: 'var(--text)',
      letterSpacing: '-0.01em',
      marginTop: 28,
      marginBottom: 8,
    }}>
      {children}
    </h3>
  );
}

function P({ children }: { children: React.ReactNode }) {
  return (
    <p style={{ color: 'var(--text-muted)', lineHeight: 1.8, marginBottom: 14, fontSize: 14.5 }}>
      {children}
    </p>
  );
}

function KW({ children }: { children: React.ReactNode }) {
  return (
    <code style={{
      fontFamily: "'Chivo Mono', monospace",
      fontSize: '0.88em',
      color: 'var(--tok-keyword)',
      background: 'var(--surface-2)',
      padding: '1px 5px',
      borderRadius: 4,
    }}>
      {children}
    </code>
  );
}

function IC({ children }: { children: React.ReactNode }) {
  return (
    <code style={{
      fontFamily: "'Chivo Mono', monospace",
      fontSize: '0.88em',
      color: 'var(--text)',
      background: 'var(--surface-2)',
      padding: '1px 5px',
      borderRadius: 4,
    }}>
      {children}
    </code>
  );
}

function Grammar({ children }: { children: string }) {
  return (
    <pre style={{
      fontFamily: "'Chivo Mono', monospace",
      fontSize: 12.5,
      lineHeight: 1.9,
      background: 'var(--surface)',
      border: '1px solid var(--border)',
      borderLeft: '3px solid var(--accent-dim)',
      borderRadius: '0 6px 6px 0',
      padding: '12px 18px',
      color: 'var(--text-muted)',
      margin: '10px 0 20px',
      overflowX: 'auto',
      whiteSpace: 'pre',
    }}>
      {children}
    </pre>
  );
}

function Note({ children }: { children: React.ReactNode }) {
  return (
    <div style={{
      background: '#a8703a0e',
      border: '1px solid #a8703a40',
      borderLeft: '3px solid var(--accent)',
      borderRadius: '0 6px 6px 0',
      padding: '10px 16px',
      margin: '14px 0 20px',
      fontSize: 14,
      color: 'var(--text-muted)',
      lineHeight: 1.75,
    }}>
      <span style={{ fontWeight: 600, color: 'var(--accent)', marginRight: 8 }}>Note.</span>
      {children}
    </div>
  );
}

function Dl({ items }: { items: [React.ReactNode, React.ReactNode][] }) {
  return (
    <dl style={{ margin: '10px 0 20px' }}>
      {items.map(([term, def], i) => (
        <div key={i} style={{ display: 'flex', gap: 20, padding: '6px 0', borderBottom: '1px solid var(--border-subtle)' }}>
          <dt style={{ minWidth: 120, fontFamily: "'Chivo Mono', monospace", fontSize: 13, color: 'var(--tok-keyword)', flexShrink: 0 }}>{term}</dt>
          <dd style={{ color: 'var(--text-muted)', fontSize: 14, lineHeight: 1.7 }}>{def}</dd>
        </div>
      ))}
    </dl>
  );
}

// ── nav structure ─────────────────────────────────────────────────────────────

const NAV = [
  { id: 'intro',         label: 'Introduction',      num: '1' },
  { id: 'lexical',       label: 'Lexical Structure',  num: '2' },
  { id: 'lex-comments',  label: 'Comments',           num: '2.1', sub: true },
  { id: 'lex-keywords',  label: 'Keywords',           num: '2.2', sub: true },
  { id: 'lex-literals',  label: 'Literals',           num: '2.3', sub: true },
  { id: 'variables',     label: 'Variables',          num: '3' },
  { id: 'expressions',   label: 'Expressions',        num: '4' },
  { id: 'expr-ops',      label: 'Operators',          num: '4.1', sub: true },
  { id: 'expr-ternary',  label: 'Ternary',            num: '4.2', sub: true },
  { id: 'expr-access',   label: 'Access',             num: '4.3', sub: true },
  { id: 'statements',    label: 'Statements',         num: '5' },
  { id: 'stmt-serve',    label: 'serve',              num: '5.1', sub: true },
  { id: 'stmt-if',       label: 'if / else',          num: '5.2', sub: true },
  { id: 'stmt-while',    label: 'while',              num: '5.3', sub: true },
  { id: 'stmt-for',      label: 'for',                num: '5.4', sub: true },
  { id: 'stmt-switch',   label: 'switch',             num: '5.5', sub: true },
  { id: 'stmt-offer',    label: 'offer',              num: '5.6', sub: true },
  { id: 'stmt-jump',     label: 'break / continue',  num: '5.7', sub: true },
  { id: 'functions',     label: 'Functions',          num: '6' },
  { id: 'fn-named',      label: 'Named functions',   num: '6.1', sub: true },
  { id: 'fn-anon',       label: 'Lambdas',            num: '6.2', sub: true },
  { id: 'fn-closures',   label: 'Closures',           num: '6.3', sub: true },
  { id: 'lists',         label: 'Lists',              num: '7' },
  { id: 'classes',       label: 'Classes',            num: '8' },
  { id: 'cls-methods',   label: 'Methods & getters',  num: '8.1', sub: true },
  { id: 'cls-static',    label: 'Static members',     num: '8.2', sub: true },
  { id: 'cls-inherit',   label: 'Inheritance',        num: '8.3', sub: true },
];

// ── page ─────────────────────────────────────────────────────────────────────

export default function SyntaxPage() {
  const [active, setActive] = useState('intro');
  const contentRef = useRef<HTMLDivElement>(null);
  const headingEls = useRef<{ id: string; el: HTMLElement }[]>([]);

  useEffect(() => {
    const el = contentRef.current;
    if (!el) return;
    const onScroll = () => {
      const threshold = 100;
      let current = 'intro';
      for (const { id, el: headEl } of headingEls.current) {
        if (headEl.getBoundingClientRect().top <= threshold) current = id;
      }
      setActive(current);
    };
    el.addEventListener('scroll', onScroll, { passive: true });
    return () => el.removeEventListener('scroll', onScroll);
  }, []);

  const scrollTo = (id: string) => {
    const entry = headingEls.current.find(h => h.id === id);
    entry?.el.scrollIntoView({ behavior: 'smooth', block: 'start' });
  };

  const regRef = (id: string) => (el: HTMLElement | null) => {
    if (!el) return;
    el.style.scrollMarginTop = '20px';
    const existing = headingEls.current.findIndex(h => h.id === id);
    if (existing >= 0) headingEls.current[existing] = { id, el };
    else headingEls.current.push({ id, el });
  };

  return (
    <div style={{ display: 'flex', height: '100%', overflow: 'hidden' }}>

      {/* ── sidebar ── */}
      <aside style={{
        width: 230,
        flexShrink: 0,
        background: 'var(--surface)',
        borderRight: '1px solid var(--border)',
        overflowY: 'auto',
        padding: '20px 0',
      }}>
        {NAV.map(item => {
          const isActive = active === item.id;
          return (
            <button
              key={item.id}
              onClick={() => scrollTo(item.id)}
              className="sidebar-btn"
              style={{
                display: 'flex',
                alignItems: 'baseline',
                gap: 8,
                width: '100%',
                textAlign: 'left',
                padding: item.sub ? '4px 18px 4px 34px' : '5px 18px',
                background: isActive ? '#e0924e18' : 'transparent',
                borderLeft: `2px solid ${isActive ? '#e0924e' : 'transparent'}`,
                color: isActive ? '#e0924e' : item.sub ? 'var(--text-faint)' : 'var(--text-muted)',
                fontSize: item.sub ? 12.5 : 13.5,
                fontWeight: isActive ? 600 : 400,
                cursor: 'pointer',
                border: 'none',
                outline: 'none',
                transition: 'color 0.1s',
                fontFamily: 'inherit',
                lineHeight: 1.6,
              }}
            >
              <span style={{
                color: isActive ? '#c07030' : 'var(--text-faint)',
                fontSize: 11,
                fontFamily: "'Chivo Mono', monospace",
                minWidth: 26,
                transition: 'color 0.1s',
              }}>
                {item.num}
              </span>
              {item.label}
            </button>
          );
        })}
      </aside>

      {/* ── content ── */}
      <div
        ref={contentRef}
        className="no-scrollbar"
        style={{ flex: 1, overflowY: 'auto', padding: '40px 56px 80px' }}
      >

        {/* 1. Introduction */}
        <div ref={regRef('intro')} id="intro" style={{ marginBottom: 52 }}>
          <h1 style={{ fontSize: 30, fontWeight: 700, letterSpacing: '-0.03em', marginBottom: 16 }}>
            Why I built it
          </h1>
          <P>
            I've always loved coding in Flutter. At some point I got curious about how
            programming languages actually work, and while looking around I found that Flutter's
            creator, Bob Nystrom, had written a book called{' '}
            <a
              href="https://craftinginterpreters.com"
              target="_blank"
              rel="noopener noreferrer"
              style={{ color: 'var(--accent)', textDecoration: 'underline', textDecorationColor: 'var(--accent)', textDecorationThickness: '1px', textUnderlineOffset: '3px' }}
            >
              Crafting Interpreters
            </a>
            . I picked it up, worked through it, and Iroh is what came out the other side.
          </P>
          <P>
            Iroh is a dynamically-typed, expression-oriented scripting language with a C-family
            syntax. I implemented it twice: once as a tree-walking interpreter in Java, and once
            as a bytecode virtual machine in C. Both share the same surface syntax and semantics.
          </P>
          <P>
            Below is a quick reference for all of Iroh's reserved keywords and what they map to
            in conventional languages.
          </P>

          <table style={{ marginTop: 6 }}>
            <thead>
              <tr><th>Keyword</th><th>Conventional equivalent</th><th>Purpose</th></tr>
            </thead>
            <tbody>
              {[
                ['brew',   'var / let',    'Declare a variable'],
                ['craft',  'fn / function','Declare or express a function'],
                ['leaf',   'class / static','Declare a class, or mark a static method'],
                ['stem',   'this / self',  'Receiver reference inside a method'],
                ['elder',  'super',        'Superclass reference inside a method body'],
                ['init',   'constructor',  'Special method called on instantiation'],
                ['offer',  'return',       'Return a value from a function'],
                ['serve',  'print',        'Print a value followed by a newline'],
                ['empty',  'null / nil',   'The absence of a value'],
              ].map(([kw, eq, desc]) => (
                <tr key={kw}>
                  <td><code style={{ color: 'var(--tok-keyword)', fontFamily: "'Chivo Mono', monospace", fontSize: 14, fontWeight: 700 }}>{kw}</code></td>
                  <td style={{ color: 'var(--text-faint)', fontFamily: "'Chivo Mono', monospace", fontSize: 13 }}>{eq}</td>
                  <td style={{ color: 'var(--text-muted)', fontSize: 13.5 }}>{desc}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>

        {/* 2. Lexical Structure */}
        <div style={{ marginBottom: 52 }}>
          <div ref={regRef('lexical')} id="lexical"><H2 id="lexical">2. Lexical Structure</H2></div>
          <P>
            Iroh source is UTF-8 text. The scanner discards whitespace and comments before
            handing tokens to the parser, always reading the longest valid token.
          </P>

          <div ref={regRef('lex-comments')}><H3 id="lex-comments">2.1 Comments</H3></div>
          <P>
            Line comments start with <IC>//</IC> and extend to the end of the line. There is
            no block-comment syntax.
          </P>
          <CodeBlock code={`// full-line comment
brew x = 1; // trailing comment`} />

          <div ref={regRef('lex-keywords')}><H3 id="lex-keywords">2.2 Keywords</H3></div>
          <P>
            These identifiers are reserved and cannot be used as variable or function names:
          </P>
          <div style={{
            display: 'flex',
            flexWrap: 'wrap',
            justifyContent: 'space-between',
            rowGap: 14,
            padding: '18px 4px',
            margin: '10px 0 20px',
            borderTop: '1px solid var(--border)',
            borderBottom: '1px solid var(--border)',
          }}>
            {['and','break','case','continue','craft','default','else','elder','empty','false','for','if','leaf','offer','or','serve','stem','switch','true','while','brew','init'].map(kw => (
              <span key={kw} style={{
                fontFamily: "'Chivo Mono', monospace",
                fontSize: 13.5,
                fontWeight: 600,
                color: 'var(--tok-keyword)',
              }}>{kw}</span>
            ))}
          </div>

          <div ref={regRef('lex-literals')}><H3 id="lex-literals">2.3 Literals</H3></div>
          <P>
            There are five kinds of literal: integers, floating-point numbers, strings,
            booleans, and the empty literal.
          </P>
          <P>
            Integers and floats are written in decimal. Strings use double quotes; single
            quotes are not valid. <KW>empty</KW> represents the absence of a value, equivalent
            to <IC>null</IC> or <IC>nil</IC> elsewhere.
          </P>
          <CodeBlock code={`brew n    = 42;
brew pi   = 3.14159;
brew name = "Iroh";
brew ok   = true;
brew none = empty;`} />
        </div>

        {/* 3. Variables */}
        <div style={{ marginBottom: 52 }}>
          <div ref={regRef('variables')}><H2 id="variables">3. Variables</H2></div>
          <P>
            Variables are declared with <KW>brew</KW>. An initialiser is optional; omit it
            and the variable starts as <KW>empty</KW>. Reassignment uses plain assignment,
            no need to repeat <KW>brew</KW>.
          </P>
          <CodeBlock code={`brew count = 0;
brew label;        // implicitly empty

count = count + 1;
label = "ready";`} />
        </div>

        {/* 4. Expressions */}
        <div style={{ marginBottom: 52 }}>
          <div ref={regRef('expressions')}><H2 id="expressions">4. Expressions</H2></div>
          <P>
            Expressions produce values. The table below lists all expression forms ordered
            from lowest to highest precedence.
          </P>

          <div ref={regRef('expr-ops')}><H3 id="expr-ops">4.1 Operators and Precedence</H3></div>
          <table style={{ marginBottom: 20 }}>
            <thead>
              <tr><th>Precedence (low → high)</th><th>Operators</th></tr>
            </thead>
            <tbody>
              {[
                ['Assignment',    '='],
                ['Ternary',       '?  :'],
                ['Logical or',    'or'],
                ['Logical and',   'and'],
                ['Equality',      '==  !='],
                ['Comparison',    '<  <=  >  >='],
                ['Additive',      '+  −'],
                ['Multiplicative','*  /'],
                ['Unary',         '!  − (prefix)'],
                ['Call / access', '()  .  []'],
              ].map(([prec, ops]) => (
                <tr key={prec}>
                  <td style={{ color: 'var(--text-muted)', fontSize: 13.5 }}>{prec}</td>
                  <td><code style={{ color: 'var(--tok-operator)', fontFamily: "'Chivo Mono', monospace", fontSize: 13 }}>{ops}</code></td>
                </tr>
              ))}
            </tbody>
          </table>

          <div ref={regRef('expr-ternary')}><H3 id="expr-ternary">4.2 Ternary Conditional</H3></div>
          <P>
            The ternary checks its first operand and returns the second or third depending on
            truthiness. Because it's right-associative, ternaries chain naturally for
            multi-branch logic.
          </P>
          <CodeBlock code={`brew score = 85;
brew grade = score >= 90 ? "A"
           : score >= 80 ? "B"
           : score >= 70 ? "C"
                        : "F";
serve grade;`} />

          <div ref={regRef('expr-access')}><H3 id="expr-access">4.3 Subscript and Attribute Access</H3></div>
          <P>
            List elements are accessed with <IC>expr[index]</IC>, object fields and methods
            with <IC>expr.name</IC>, and functions are called with <IC>expr(args)</IC>. These
            forms chain in any order.
          </P>
          <CodeBlock code={`serve matrix[1][0];      // nested subscript
serve obj.field;         // attribute
serve obj.method(x, y); // method call
serve arr[0].name;       // subscript then attribute`} />
        </div>

        {/* 5. Statements */}
        <div style={{ marginBottom: 52 }}>
          <div ref={regRef('statements')}><H2 id="statements">5. Statements</H2></div>
          <P>
            Every statement, including expression statements and variable declarations,
            must end with a semicolon.
          </P>

          <div ref={regRef('stmt-serve')}><H3 id="stmt-serve">5.1 The serve Statement</H3></div>
          <P>
            <KW>serve</KW> prints a value to standard output, followed by a newline.
          </P>
          <CodeBlock code={`serve "hello";
serve 1 + 2;
serve true;`} />

          <div ref={regRef('stmt-if')}><H3 id="stmt-if">5.2 The if Statement</H3></div>
          <P>
            <KW>if</KW> conditionally executes a block, with an optional <KW>else</KW>{' '}
            branch for when the condition is falsy. The condition must be wrapped in parentheses.
          </P>
          <CodeBlock code={`if (x > 0) {
  serve "positive";
} else {
  serve "non-positive";
}`} />

          <div ref={regRef('stmt-while')}><H3 id="stmt-while">5.3 The while Statement</H3></div>
          <P>
            <KW>while</KW> repeats a block as long as a condition stays truthy.
          </P>
          <CodeBlock code={`brew i = 0;
while (i < 5) {
  serve i;
  i = i + 1;
}`} />

          <div ref={regRef('stmt-for')}><H3 id="stmt-for">5.4 The for Statement</H3></div>
          <P>
            <KW>for</KW> provides C-style loops. All three clauses (initialiser, condition,
            increment) are optional; omitting the condition creates an infinite loop.
          </P>
          <CodeBlock code={`for (brew j = 0; j < 3; j = j + 1) {
  serve j;
}`} />

          <div ref={regRef('stmt-switch')}><H3 id="stmt-switch">5.5 The switch Statement</H3></div>
          <P>
            <KW>switch</KW> compares a value against a series of <KW>case</KW> labels by
            equality. An optional <KW>default</KW> handles anything unmatched. Unlike C,
            cases don't fall through.
          </P>
          <CodeBlock code={`switch (color) {
  case "red":   serve 1;
  case "green": serve 2;
  default:      serve 0;
}`} />

          <div ref={regRef('stmt-offer')}><H3 id="stmt-offer">5.6 The offer Statement</H3></div>
          <P>
            <KW>offer</KW> exits the enclosing <KW>craft</KW> and returns a value to the
            caller. A bare <KW>offer ;</KW> returns <KW>empty</KW>.
          </P>
          <CodeBlock code={`craft sign(n) {
  if (n > 0) { offer  1; }
  if (n < 0) { offer -1; }
  offer 0;
}`} />

          <div ref={regRef('stmt-jump')}><H3 id="stmt-jump">5.7 break and continue</H3></div>
          <P>
            <KW>break</KW> exits the nearest enclosing loop immediately.{' '}
            <KW>continue</KW> skips the rest of the current iteration and moves to the next.
            Both are only valid inside a <KW>while</KW> or <KW>for</KW> body.
          </P>
          <CodeBlock code={`brew i = 0;
while (true) {
  if (i >= 3) { break; }
  serve i;
  i = i + 1;
}`} />
        </div>

        {/* 6. Functions */}
        <div style={{ marginBottom: 52 }}>
          <div ref={regRef('functions')}><H2 id="functions">6. Functions</H2></div>
          <P>
            Functions are first-class values in Iroh: passable as arguments, returnable
            from other functions, and storable in variables.
          </P>

          <div ref={regRef('fn-named')}><H3 id="fn-named">6.1 Named Function Declarations</H3></div>
          <P>
            Named functions are declared with <KW>craft</KW>. The name is bound in the
            enclosing scope, whether at the top level or inside a block.
          </P>
          <CodeBlock code={`craft add(a, b) {
  offer a + b;
}

serve add(3, 4);`} />

          <div ref={regRef('fn-anon')}><H3 id="fn-anon">6.2 Anonymous Functions (Lambdas)</H3></div>
          <P>
            A <KW>craft</KW> without a name produces an anonymous function usable anywhere
            an expression is expected. When assigned to a variable, the statement still
            needs its trailing semicolon.
          </P>
          <CodeBlock code={`brew square = craft(n) { offer n * n; };

serve square(5);`} />

          <div ref={regRef('fn-closures')}><H3 id="fn-closures">6.3 Closures</H3></div>
          <P>
            Every function captures its enclosing environment at creation time, making it
            possible to return an inner function that still references outer variables.
            That's a closure.
          </P>
          <CodeBlock code={`craft makeAdder(n) {
  offer craft(x) { offer x + n; };
}

brew addFive = makeAdder(5);
serve addFive(10);
serve addFive(3);`} />
          <P>
            This naturally extends to higher-order functions, passed as arguments just like
            any other value.
          </P>
          <CodeBlock code={`craft apply(fn, val) {
  offer fn(val);
}

serve apply(craft(n) { offer n * 2; }, 6);`} />
        </div>

        {/* 7. Lists */}
        <div style={{ marginBottom: 52 }}>
          <div ref={regRef('lists')}><H2 id="lists">7. Lists</H2></div>
          <P>
            Lists are created with a bracket literal. They're heterogeneous, zero-indexed,
            and mutable. Elements can be read and written via subscript.
          </P>
          <CodeBlock code={`brew nums = [10, 20, 30];
serve nums[0];
nums[1] = 99;
serve nums[1];`} />
          <P>
            Lists can be nested and subscripts chained:
          </P>
          <CodeBlock code={`brew matrix = [[1, 2], [3, 4]];
serve matrix[1][0];`} />
          <P>
            Lists hold mixed types freely. There's currently no built-in length property;
            the Java interpreter supports dynamic length inspection.
          </P>
        </div>

        {/* 8. Classes */}
        <div style={{ marginBottom: 52 }}>
          <div ref={regRef('classes')}><H2 id="classes">8. Classes</H2></div>
          <P>
            Classes are declared with <KW>leaf</KW>. Instances are created by calling the
            class like a function; <KW>init</KW> receives the arguments.
          </P>
          <div ref={regRef('cls-methods')}><H3 id="cls-methods">8.1 Instance Methods and Property Getters</H3></div>
          <P>
            Inside any instance member, <KW>stem</KW> refers to the current object. A member
            with a parameter list is a regular method called as <IC>obj.method(args)</IC>.
            A member declared without a parameter list is a <em>property getter</em>, accessed
            without parentheses as <IC>obj.property</IC>.
          </P>
          <CodeBlock code={`leaf Circle {
  init(r) {
    stem.r = r;
  }
  area     { offer 3.14159 * stem.r * stem.r; }
  diameter { offer stem.r * 2; }
  scale(factor) { stem.r = stem.r * factor; }
}

brew c = Circle(5);
serve c.area;
serve c.diameter;
c.scale(2);
serve c.diameter;`} />

          <div ref={regRef('cls-static')}><H3 id="cls-static">8.2 Static Members</H3></div>
          <P>
            Prefixing a member with <KW>leaf</KW> makes it static, callable on the class
            itself rather than on an instance.
          </P>
          <CodeBlock code={`leaf MathUtils {
  leaf square(n)   { offer n * n; }
  leaf max(a, b)   { offer a > b ? a : b; }
}

serve MathUtils.square(6);
serve MathUtils.max(10, 3);`} />

          <div ref={regRef('cls-inherit')}><H3 id="cls-inherit">8.3 Inheritance</H3></div>
          <P>
            Inheritance is declared with <IC>{'<'}</IC> after the class name. Inside the
            subclass, <KW>elder</KW> followed by a dot accesses the superclass's methods.
          </P>
          <CodeBlock code={`leaf Animal {
  init(name) {
    stem.name = name;
  }
  speak { offer stem.name + " makes a sound"; }
}

leaf Dog < Animal {
  init(name) {
    stem.name = name;
  }
  speak { offer elder.speak + " woof!"; }
}

brew d = Dog("Rex");
serve d.speak;`} />
          <P>
            Only single inheritance is supported. A subclass that defines <IC>init</IC> must
            manually set any fields it wants from the superclass.
          </P>
        </div>

      </div>
    </div>
  );
}
