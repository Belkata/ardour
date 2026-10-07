// usage: node render.js <in.html> <out.png> [w] [h] [--clean]
const { chromium } = require(process.env.PW_MOD || 'playwright');
(async () => {
  const [inp, out, w = 1680, h = 1000] = process.argv.slice(2).filter(a => !a.startsWith('--'));
  const clean = process.argv.includes('--clean');
  const b = await chromium.launch({ executablePath: '/opt/pw-browsers/chromium-1194/chrome-linux/chrome' });
  const p = await b.newPage({ viewport: { width: +w, height: +h }, deviceScaleFactor: 1.5 });
  const [f, h2] = inp.split('#');
  await p.goto('file://' + require('path').resolve(f) + (h2 ? '#' + h2 : ''));
  if (clean) await p.addStyleTag({ content: '.co{display:none}' });
  await p.waitForTimeout(300);
  await p.screenshot({ path: out });
  await b.close();
})();
