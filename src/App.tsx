import { useState } from 'react';
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import { ThemeProvider, useTheme } from './theme';
import Navbar from './components/Navbar';
import SyntaxPage from './pages/SyntaxPage';
import PlaygroundPage from './pages/PlaygroundPage';

function SunIcon() {
  return (
    <svg width="17" height="17" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
      <circle cx="12" cy="12" r="4" />
      <line x1="12" y1="2" x2="12" y2="4" />
      <line x1="12" y1="20" x2="12" y2="22" />
      <line x1="4.22" y1="4.22" x2="5.64" y2="5.64" />
      <line x1="18.36" y1="18.36" x2="19.78" y2="19.78" />
      <line x1="2" y1="12" x2="4" y2="12" />
      <line x1="20" y1="12" x2="22" y2="12" />
      <line x1="4.22" y1="19.78" x2="5.64" y2="18.36" />
      <line x1="18.36" y1="5.64" x2="19.78" y2="4.22" />
    </svg>
  );
}

function MoonIcon() {
  return (
    <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
      <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z" />
    </svg>
  );
}

function ThemeToggle() {
  const { isDark, toggle } = useTheme();
  const [spinning, setSpinning] = useState(false);

  function handleClick() {
    setSpinning(true);
    toggle();
  }

  return (
    <button
      onClick={handleClick}
      onAnimationEnd={() => setSpinning(false)}
      className={spinning ? 'theme-spin' : ''}
      title={isDark ? 'Switch to light mode' : 'Switch to dark mode'}
      style={{
        position: 'fixed',
        bottom: 24,
        right: 24,
        zIndex: 100,
        width: 44,
        height: 44,
        borderRadius: '50%',
        border: 'none',
        background: '#e0924e',
        color: '#1a1a1a',
        cursor: 'pointer',
        outline: 'none',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        boxShadow: '0 4px 16px rgba(0,0,0,0.3)',
        transition: 'color 0.12s, background 0.12s',
      }}
      onMouseEnter={e => {
        (e.currentTarget as HTMLButtonElement).style.background = '#f0a060';
      }}
      onMouseLeave={e => {
        (e.currentTarget as HTMLButtonElement).style.background = '#e0924e';
      }}
    >
      {isDark ? <SunIcon /> : <MoonIcon />}
    </button>
  );
}

export default function App() {
  return (
    <ThemeProvider>
      <BrowserRouter>
        <div style={{ display: 'flex', flexDirection: 'column', height: '100vh', overflow: 'hidden' }}>
          <Navbar />
          <div style={{ flex: 1, overflow: 'hidden', display: 'flex', flexDirection: 'column' }}>
            <Routes>
              <Route path="/" element={<Navigate to="/playground" replace />} />
              <Route path="/syntax" element={<SyntaxPage />} />
              <Route path="/playground" element={<PlaygroundPage />} />
            </Routes>
          </div>
          <ThemeToggle />
        </div>
      </BrowserRouter>
    </ThemeProvider>
  );
}
