import { createContext, useContext, useEffect, useState } from 'react';

interface ThemeCtx { isDark: boolean; toggle: () => void }

const Ctx = createContext<ThemeCtx>({ isDark: true, toggle: () => {} });

export function ThemeProvider({ children }: { children: React.ReactNode }) {
  const [isDark, setIsDark] = useState(() => {
    const saved = localStorage.getItem('iroh-theme');
    if (saved) return saved === 'dark';
    return window.matchMedia('(prefers-color-scheme: dark)').matches;
  });

  useEffect(() => {
    document.documentElement.dataset.theme = isDark ? 'dark' : 'light';
    localStorage.setItem('iroh-theme', isDark ? 'dark' : 'light');
  }, [isDark]);

  return (
    <Ctx.Provider value={{ isDark, toggle: () => setIsDark(d => !d) }}>
      {children}
    </Ctx.Provider>
  );
}

export const useTheme = () => useContext(Ctx);
