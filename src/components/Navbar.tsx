import { NavLink } from 'react-router-dom';

export default function Navbar() {
  return (
    <nav style={{
      display: 'flex',
      alignItems: 'center',
      gap: 6,
      padding: '0 32px 0 4px',
      height: 76,
      background: 'var(--surface)',
      flexShrink: 0,
      userSelect: 'none',
    }}>
      <NavLink to="/playground" className={({ isActive }) => `nav-link${isActive ? ' active' : ''}`}>Playground</NavLink>
      <NavLink to="/syntax"     className={({ isActive }) => `nav-link${isActive ? ' active' : ''}`}>Reference</NavLink>

      <div style={{ flex: 1 }} />

      <span style={{
        fontFamily: "'Chivo Mono', monospace",
        fontWeight: 900,
        fontSize: 32,
        color: '#e0924e',
        letterSpacing: '-0.04em',
        lineHeight: 1,
      }}>
        IROH
      </span>
    </nav>
  );
}
