// Graba el vídeo de los modelos 3D a partir de video/grabar.html con Edge o Chrome sin ventana.
// Uso (desde la carpeta del proyecto):  node video/grabar_video.js [salida.webm]
// No necesita librerías: sirve la carpeta por http://127.0.0.1 (WebCodecs exige un contexto
// seguro) y habla con el navegador por el protocolo de DevTools (CDP).
'use strict';
const { spawn } = require('child_process');
const fs = require('fs');
const http = require('http');
const os = require('os');
const path = require('path');

const RAIZ = path.resolve(__dirname, '..');
const salida = path.resolve(process.argv[2] || path.join(__dirname, 'modelos-3d.webm'));
const NAVEGADORES = [
  'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  'C:/Program Files/Microsoft/Edge/Application/msedge.exe',
  'C:/Program Files/Google/Chrome/Application/chrome.exe',
];
const exe = NAVEGADORES.find(p => fs.existsSync(p));
if (!exe) { console.error('No encuentro Edge ni Chrome'); process.exit(1); }
const PUERTO_WEB = 8765, PUERTO_CDP = 9339;
const TIPOS = { '.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8', '.png': 'image/png' };
const esperar = ms => new Promise(r => setTimeout(r, ms));

const servidor = http.createServer((req, res) => {
  const ruta = path.join(RAIZ, decodeURIComponent(req.url.split('?')[0]));
  if (!ruta.startsWith(RAIZ) || !fs.existsSync(ruta) || fs.statSync(ruta).isDirectory()) { res.writeHead(404); return res.end(); }
  res.writeHead(200, { 'Content-Type': TIPOS[path.extname(ruta)] || 'application/octet-stream', 'Cache-Control': 'no-store' });
  fs.createReadStream(ruta).pipe(res);
});

(async () => {
  // Si ya responde algo en el puerto de depuración, es un navegador de otra grabación
  try { await fetch(`http://127.0.0.1:${PUERTO_CDP}/json/version`); throw new Error(`El puerto ${PUERTO_CDP} está ocupado por otro navegador sin ventana: ciérralo y repite`); }
  catch (e) { if (e.message.startsWith('El puerto')) throw e; }
  await new Promise(ok => servidor.listen(PUERTO_WEB, '127.0.0.1', ok));
  const url = `http://127.0.0.1:${PUERTO_WEB}/video/grabar.html`;
  const perfil = fs.mkdtempSync(path.join(os.tmpdir(), 'edge-grabacion-'));   // perfil limpio: sin caché vieja
  const nav = spawn(exe, [
    '--headless=new', `--remote-debugging-port=${PUERTO_CDP}`, `--user-data-dir=${perfil}`,
    '--window-size=1280,720', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--hide-scrollbars',
    '--disable-background-timer-throttling', '--disable-renderer-backgrounding', '--disable-backgrounding-occluded-windows',
    url,
  ], { stdio: 'ignore' });

  try {
    let pagina = null;
    for (let i = 0; i < 60 && !pagina; i++) {
      try {
        const lista = await (await fetch(`http://127.0.0.1:${PUERTO_CDP}/json/list`)).json();
        pagina = lista.find(p => p.type === 'page' && p.url.includes('grabar.html'));
      } catch { /* el navegador aún arranca */ }
      if (!pagina) await esperar(500);
    }
    if (!pagina) throw new Error('El navegador no abrió la página');

    const ws = new WebSocket(pagina.webSocketDebuggerUrl);
    const pendientes = new Map();
    let n = 0;
    ws.onmessage = ev => {
      const m = JSON.parse(typeof ev.data === 'string' ? ev.data : ev.data.toString());
      if (m.id && pendientes.has(m.id)) { pendientes.get(m.id)(m); pendientes.delete(m.id); }
    };
    await new Promise((ok, mal) => { ws.onopen = ok; ws.onerror = mal; });
    const enviar = (method, params) => new Promise(ok => { const id = ++n; pendientes.set(id, ok); ws.send(JSON.stringify({ id, method, params })); });
    const evaluar = async (expr, espera = false) => {
      const r = await enviar('Runtime.evaluate', { expression: expr, awaitPromise: espera, returnByValue: true });
      if (r.result.exceptionDetails) throw new Error(JSON.stringify(r.result.exceptionDetails));
      return r.result.result.value;
    };

    for (let i = 0; i < 60 && !(await evaluar('window.listo === true')); i++) await esperar(500);
    const d = JSON.parse(await evaluar("JSON.stringify({ webcodecs: typeof VideoEncoder !== 'undefined', muxer: !!window.WebMMuxer, errores: window.errores })"));
    const fotograma = d.webcodecs && d.muxer;
    console.log(fotograma ? 'Grabando fotograma a fotograma a 30 fps (tarda un par de minutos)…'
                          : `Sin WebCodecs (${JSON.stringify(d)}): grabando en tiempo real (45 s)…`);
    const res = JSON.parse(await evaluar(fotograma ? 'grabarFotogramas()' : 'grabar()', true));
    fs.writeFileSync(salida, Buffer.from(res.datos, 'base64'));
    console.log(`Listo: ${salida} · ${(fs.statSync(salida).size / 1e6).toFixed(1)} MB · ${res.tipo} · ${res.fps.toFixed(1)} fotogramas/s`);
    ws.close();
  } finally {
    // Cerrar el navegador entero por CDP (matar el proceso padre deja hijos vivos en Windows)
    try {
      const v = await (await fetch(`http://127.0.0.1:${PUERTO_CDP}/json/version`)).json();
      const b = new WebSocket(v.webSocketDebuggerUrl);
      await new Promise(ok => { b.onopen = ok; b.onerror = ok; });
      b.send(JSON.stringify({ id: 1, method: 'Browser.close' }));
      await esperar(800);
    } catch { /* ya estaba cerrado */ }
    nav.kill();
    servidor.close();
    setTimeout(() => { try { fs.rmSync(perfil, { recursive: true, force: true }); } catch { /* aún en uso */ } }, 1500);
  }
})().catch(e => { console.error(e.message || e); process.exit(1); });
