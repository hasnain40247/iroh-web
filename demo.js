import { chromium } from 'playwright';

const PORT = 5175;
const URL  = `http://localhost:${PORT}/playground`;

const CHUNKS = [
  `leaf Shape {
  init(name) {
    stem.name = name;
  }
  describe {
    offer "I am a " + stem.name;
  }
}`,

  `leaf Circle < Shape {
  init(r) {
    stem.name = "circle";
    stem.r = r;
  }
  area {
    offer 3.14159 * stem.r * stem.r;
  }
  diameter {
    offer stem.r * 2;
  }
}`,

  `leaf Rectangle < Shape {
  init(w, h) {
    stem.name = "rectangle";
    stem.w = w;
    stem.h = h;
  }
  area {
    offer stem.w * stem.h;
  }
  perimeter {
    offer 2 * (stem.w + stem.h);
  }
}`,

  `craft makeScaler(factor) {
  offer craft(shape) {
    offer shape.area * factor;
  };
}

brew c = Circle(5);
brew r = Rectangle(4, 6);

serve c.describe;
serve c.area;
serve c.diameter;

serve r.describe;
serve r.area;
serve r.perimeter;

brew doubleArea = makeScaler(2);
serve doubleArea(c);
serve doubleArea(r);`,
];

async function typeLines(page, code) {
  const lines = code.split('\n');
  for (let i = 0; i < lines.length; i++) {
    const content = lines[i].trimStart();
    for (const char of content) {
      await page.keyboard.type(char, { delay: 90 });
    }
    if (i < lines.length - 1) {
      await page.keyboard.press('Enter');
      await page.waitForTimeout(40);
    }
  }
}

async function scrollBuffer(page, count = 7) {
  // push viewport down
  for (let i = 0; i < count; i++) {
    await page.keyboard.press('Enter');
    await page.waitForTimeout(25);
  }
  await page.waitForTimeout(200);
  // come back up to where we left off
  for (let i = 0; i < count; i++) {
    await page.keyboard.press('ArrowUp');
    await page.waitForTimeout(20);
  }
  // move to end of that line so next Enter goes to a fresh line
  await page.keyboard.press('End');
  await page.waitForTimeout(150);
}

(async () => {
  const browser = await chromium.launch({
    executablePath: '/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge',
    headless: false,
    args: ['--start-fullscreen'],
  });

  const ctx  = await browser.newContext({ viewport: null });
  const page = await ctx.newPage();

  await page.goto(URL);
  await page.waitForSelector('.monaco-editor', { timeout: 10000 });
  await page.waitForTimeout(1200);

  await page.selectOption('select', 'c');
  await page.waitForTimeout(400);

  const editor = page.locator('.monaco-editor').first();
  await editor.click();
  await page.keyboard.press('Meta+A');
  await page.keyboard.press('Backspace');
  await page.waitForTimeout(400);

  for (let i = 0; i < CHUNKS.length; i++) {
    await typeLines(page, CHUNKS[i]);

    if (i < CHUNKS.length - 1) {
      // blank line between chunks then scroll buffer
      await page.keyboard.press('Enter');
      await page.waitForTimeout(60);
      await scrollBuffer(page, 8);
      await page.keyboard.press('Enter');
      await page.waitForTimeout(60);
    }
  }

  await page.waitForTimeout(900);
  await page.click('button[title="Run (⌘ Enter)"]');
  await page.waitForTimeout(3500);

  console.log('Demo complete. Close the browser when you finish recording.');
})();
