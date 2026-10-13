// Núcleo del simulador: hidráulica del dispositivo + el mismo algoritmo que el firmware.
// Unidades: alturas en mm, volúmenes en mL, caudales en mL/s, tiempo en s.
(function (root) {
  'use strict';

  const G = 9.81;

  // ---------- Hidráulica ----------

  // Abertura rectangular de ancho b (mm) que va de z=0 a z=alto (mm), con carga h (mm).
  // Fórmula de vertedero de pared delgada: Q = 2/3·Cd·b·√(2g)·h^1.5
  function aberturaQ(h, b, alto, cd) {
    if (h <= 0) return 0;
    const k = (2 / 3) * cd * (b / 1000) * Math.sqrt(2 * G) * 1e6;
    const hi = h / 1000;
    const lo = Math.max(0, h - alto) / 1000;
    return k * (Math.pow(hi, 1.5) - Math.pow(lo, 1.5));
  }

  // Orificio circular de diámetro d (mm) con carga h (mm).
  function orificioQ(h, d, cd) {
    if (h <= 0) return 0;
    const A = Math.PI / 4 * Math.pow(d / 1000, 2);
    return cd * A * Math.sqrt(2 * G * h / 1000) * 1e6;
  }

  // Coeficiente "a" de la ley Q = a·h^n (h en mm) equivalente a la ranura teórica.
  function coefRanura(b, cd) {
    return (2 / 3) * cd * (b / 1000) * Math.sqrt(2 * G) * 1e6 / Math.pow(1000, 1.5);
  }

  // Varias ranuras repartidas alrededor de la cámara. dc = profundidad (mm) del centro del
  // suelo bajo la superficie, medida en la dirección de la gravedad; up = vector "arriba"
  // en ejes del aparato; g = gravedad aparente en g. Una ranura inclinada dentro de su
  // propio plano se ve más ancha en horizontal (factor 1/cos α).
  function eje(f) { return [Math.cos(f), Math.sin(f)]; }
  function caudalModelo(dc, up, g, geo, a, n, hMin) {
    let q = 0;
    for (let i = 0; i < geo.nRan; i++) {
      const [c, s] = eje(geo.fase + i * 2 * Math.PI / geo.nRan);
      const di = dc - geo.r * (up[0] * c + up[1] * s);
      if (di <= hMin) continue;
      const tanA = (-up[0] * s + up[1] * c) / up[2];
      q += a / geo.nRan * Math.sqrt(g) * Math.pow(di, n) * Math.sqrt(1 + tanA * tanA);
    }
    return q;
  }

  // ---------- Perfiles de micción ----------

  function campana(x, a, b) {
    if (x <= 0 || x >= 1) return 0;
    return Math.pow(x, a) * Math.pow(1 - x, b);
  }
  function suave(e0, e1, x) {
    const t = Math.min(1, Math.max(0, (x - e0) / (e1 - e0)));
    return t * t * (3 - 2 * t);
  }
  function hueco(x, c, w) {
    return 1 - suave(c - w - 0.025, c - w, x) * (1 - suave(c + w, c + w + 0.025, x));
  }

  const PERFILES = {
    normal: {
      nombre: 'Normal (hombre)', qmax: 25, vol: 300,
      forma: x => campana(x, 1.1, 2.3),
    },
    mujer: {
      nombre: 'Normal (mujer)', qmax: 32, vol: 300,
      forma: x => campana(x, 0.8, 2.2),
    },
    obstructivo: {
      nombre: 'Obstructivo (próstata)', qmax: 8, vol: 250,
      forma: x => (x <= 0 || x >= 1) ? 0 : suave(0, 0.12, x) * (1 - 0.18 * x) * (1 - suave(0.72, 1, x)),
    },
    intermitente: {
      nombre: 'Intermitente (esfuerzo)', qmax: 18, vol: 300,
      forma: x => campana(x, 1, 1.6) * hueco(x, 0.3, 0.035) * hueco(x, 0.6, 0.04) *
        (0.8 + 0.2 * Math.sin(x * Math.PI * 7)),
    },
    extremo: {
      nombre: 'Pico máximo', qmax: 50, vol: 500,
      forma: x => campana(x, 0.8, 2.0),
    },
  };

  function mulberry32(seed) {
    let a = seed >>> 0;
    return function () {
      a = (a + 0x6D2B79F5) >>> 0;
      let t = a;
      t = Math.imul(t ^ (t >>> 15), t | 1);
      t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
  }

  // Genera el caudal de entrada q[i] (mL/s) muestreado cada dt.
  function generarFlujo(perfil, vol, qmax, dt, semilla) {
    const p = PERFILES[perfil];
    const N = 4000;
    let pico = 0, suma = 0;
    for (let i = 0; i <= N; i++) {
      const s = p.forma(i / N);
      if (s > pico) pico = s;
      suma += s;
    }
    const media = suma / (N + 1) / pico;
    const goteo = Math.min(3, vol * 0.01);
    const vPrincipal = vol - goteo;
    const T = vPrincipal / (qmax * media);
    const t0 = 1.0;
    const total = t0 + T + 60;   // margen: la simulación corta antes si ya terminó
    const n = Math.ceil(total / dt);
    const q = new Float64Array(n);
    const rng = mulberry32(semilla || 7);
    const pasoRuido = Math.max(1, Math.round(0.4 / dt));
    let ruido = 0, objetivo = 0, s = 0;
    for (let i = 0; i < n; i++) {
      if (i % pasoRuido === 0) objetivo = rng() * 2 - 1;
      ruido += (objetivo - ruido) * dt / 0.3;
      const x = (i * dt - t0) / T;
      const v = (x > 0 && x < 1) ? p.forma(x) / pico * qmax * (1 + 0.06 * ruido) : 0;
      q[i] = Math.max(0, v);
      s += q[i] * dt;
    }
    const k = vPrincipal / s;
    for (let i = 0; i < n; i++) q[i] *= k;
    // Goteo terminal: tres pequeños chorros tras el final
    const dur = 0.3;
    [0.9, 2.0, 3.4].forEach(dtGota => {
      const i0 = Math.round((t0 + T + dtGota) / dt);
      const m = Math.round(dur / dt);
      for (let j = 0; j < m && i0 + j < n; j++) q[i0 + j] += goteo / 3 / dur;
    });
    return { q, dt, t0, T, total, n };
  }

  // Qmax según la regla de 2 s de la ICS: media móvil de 2 s sobre bloques de 0,1 s.
  function crearQmax() {
    const anillo = [];
    let acum = 0, tAcum = 0, max = 0, tMax = 0, t = 0;
    return {
      add(q, dt) {
        acum += q * dt; tAcum += dt; t += dt;
        if (tAcum >= 0.1 - 1e-9) {
          anillo.push(acum / tAcum);
          if (anillo.length > 20) anillo.shift();
          acum = 0; tAcum = 0;
          if (anillo.length === 20) {
            const m = anillo.reduce((x, y) => x + y, 0) / 20;
            if (m > max) { max = m; tMax = t - 1; }
          }
        }
      },
      get max() { return max; },
      get tMax() { return tMax; },
    };
  }

  // ---------- Firmware (mismo algoritmo que uroflujometro.ino) ----------

  function crearFirmware(cfg) {
    const c = Object.assign({
      a: 0.1737, n: 1.5, rho: 1.015,
      hInicio: 2.0, hFin: 1.0, tFin: 5.0, hMinQ: 0.3, media: 8,
      hAviso: 65, hRebose: 75, vMinGuardar: 20, hSeco: 0.4,
    }, cfg);
    const buf = [], bufA = [];
    let estado = 'LISTO', inclMax = 0, incl = 0;
    let vol = 0, tFlujo = 0, tCalma = 0, tIni = 0, tUlt = 0, rebose = false, aviso = false;
    let qmax = crearQmax();
    let resultado = null;

    function muestra(hw, t, dt, imu) {
      buf.push(hw);
      if (buf.length > c.media) buf.shift();
      const hf = buf.reduce((x, y) => x + y, 0) / buf.length;
      const hu = Math.max(0, hf) / c.rho;
      let q = hu > c.hMinQ ? c.a * Math.pow(hu, c.n) : 0;
      if (imu && c.geo) {
        bufA.push(imu);
        if (bufA.length > c.media) bufA.shift();
        const m = [0, 1, 2].map(k => bufA.reduce((x, v) => x + v[k], 0) / bufA.length);
        const g = Math.hypot(m[0], m[1], m[2]);
        const up = m.map(v => v / g);
        incl = Math.acos(Math.min(1, up[2])) * 180 / Math.PI;
        if (c.corregir) {
          const [ct, st] = eje(c.geo.phiT);
          const dToma = Math.max(0, hf) / (c.rho * g);
          // Con la toma seca no se sabe dónde está el agua: se toma la cámara por vacía
          const dc = dToma + c.geo.rT * (up[0] * ct + up[1] * st);
          q = dToma > c.hSeco ? caudalModelo(dc, up, g, c.geo, c.a, c.n, c.hMinQ) : 0;
        }
      }

      if (estado === 'LISTO' && hf > c.hInicio) {
        estado = 'MIDIENDO';
        vol = 0; tFlujo = 0; tCalma = 0; tIni = t; tUlt = t;
        rebose = false; aviso = false; qmax = crearQmax(); inclMax = 0;
      }
      if (estado === 'MIDIENDO') {
        vol += q * dt;
        if (incl > inclMax) inclMax = incl;
        qmax.add(q, dt);
        if (q > 0.5) { tFlujo += dt; tUlt = t; }
        if (hf > c.hAviso) aviso = true;
        if (hw > c.hRebose) rebose = true;   // sin filtrar, como el firmware
        tCalma = hf < c.hFin ? tCalma + dt : 0;
        if (tCalma >= c.tFin || t - tIni > 300) {
          estado = vol >= c.vMinGuardar ? 'RESULTADO' : 'LISTO';
          resultado = {
            vol, qmax: qmax.max, tQmax: qmax.tMax,
            qmed: tFlujo > 0 ? vol / tFlujo : 0,
            tMiccion: tUlt - tIni, tFlujo, rebose, aviso,
            inclMax, inclinado: inclMax > (c.inclMax || 25),
          };
        }
      }
      return { hf, q, estado, vol, incl };
    }

    return {
      muestra,
      get estado() { return estado; },
      get resultado() { return resultado; },
      get vol() { return vol; },
    };
  }

  // ---------- Diseños ----------

  const DISENOS = {
    compacto: 'Compacto portátil v0.2',
    propuesto: 'Cámara + ranura (propuesto)',
    original: 'Sensor en línea de la imagen',
    turbina: 'Turbina tipo YF-S201',
  };

  const POR_DEFECTO = {
    camD: 32,      // diámetro interior de la cámara (mm)
    b: 3,          // ancho de la ranura (mm)
    ranuraH: 70,   // alto de la ranura (mm)
    reboseH: 78,   // altura de las ventanas de rebose interno (mm)
    ventanaW: 20,  // ancho total de las ventanas de rebose (2 × 10 mm)
    camH: 95,      // alto interior de la cámara (mm)
    embudoV: 80,   // volumen útil del embudo (mL)
    cd: 0.62,      // coeficiente de descarga
    rhoReal: 1.015,
    rhoFw: 1.015,
    ruido: 0.1,    // ruido del sensor (mm de columna, desviación típica)
  };

  // Versión 0.2 portátil: 4 ranuras de 1,5 mm, toma de presión entre dos ranuras, acelerómetro
  const COMPACTO = {
    camD: 24, buzoExt: 14, buzoInt: 11, camH: 48, embudoV: 90,
    nRan: 4, b: 1.5, ranuraH: 36, reboseH: 40, ventanaW: 6, nVent: 4,
    r: 12, rT: 0, phiT: 0,   // toma en el centro, protegida bajo el tapón del buzo
    incl: 15,       // inclinación media (grados)
    vaiven: 5,      // amplitud del balanceo de la mano (grados)
    correccion: true,
    errCal: 2,      // error residual de calibración (%)
    imuErr: 0.5,    // desalineación del acelerómetro tras nivelar (grados)
  };

  function simular(diseno, perfil, vol, qmax, P) {
    if (diseno === 'compacto') return simularCompacto(perfil, vol, qmax, P);
    P = Object.assign({}, POR_DEFECTO, P || {});
    const dt = 0.005;
    const flujo = generarFlujo(perfil, vol, qmax, dt, 11);
    const fs = 40;                         // muestreo del sensor (Hz)
    const pasoSensor = Math.round(1 / fs / dt);
    const pasoSerie = Math.round(0.05 / dt); // series de 20 Hz para gráficas
    const rng = mulberry32(99);
    const gauss = () => {
      const u = Math.max(1e-12, rng()), v = rng();
      return Math.sqrt(-2 * Math.log(u)) * Math.cos(2 * Math.PI * v);
    };

    const serie = { t: [], qin: [], qmed: [], qsal: [], nivel: [], estado: [], vol: [], derr: [] };
    const qmaxReal = crearQmax();
    const qmaxMed = crearQmax();
    let S = 0, derramado = 0, perdidoRebose = 0, hMax = 0, tDesborde = null, vMed = 0, qSalMax = 0;
    let qMedActual = 0, estadoActual = 'LISTO', volActual = 0;
    let fw = null, limite, nivelAviso, nivelRebose, unidadNivel = 'mm';

    const A = Math.PI / 4 * P.camD * P.camD / 1000; // mL por mm de altura
    const Vcam = A * P.camH;
    const embudoH = 60;
    const VembudoOrig = 60;

    if (diseno === 'propuesto') {
      fw = crearFirmware({
        a: coefRanura(P.b, P.cd), n: 1.5, rho: P.rhoFw,
        hAviso: P.ranuraH - 5, hRebose: P.reboseH,
      });
      limite = P.camH + embudoH;  // nivel al que se derrama por arriba del embudo
      nivelAviso = P.ranuraH;
      nivelRebose = P.reboseH;
    } else {
      limite = embudoH;
      nivelAviso = null;
      nivelRebose = null;
    }

    function nivelDe(S) {
      if (diseno === 'propuesto') {
        if (S <= Vcam) return S / A;
        return P.camH + (S - Vcam) / P.embudoV * embudoH;
      }
      return embudoH * Math.cbrt(Math.max(0, S) / VembudoOrig); // embudo cónico
    }
    const Smax = diseno === 'propuesto' ? Vcam + P.embudoV : VembudoOrig;

    let tFin = 0;
    for (let i = 0; i < flujo.n; i++) {
      const t = i * dt;
      // Se para 12 s después del chorro, o cuando el firmware cierra la medida
      if (t > flujo.t0 + flujo.T + 12 && (!fw || fw.estado !== 'MIDIENDO')) break;
      tFin = t;
      const qin = flujo.q[i];
      const h = nivelDe(S);
      let qsal = 0, qmedInst = 0;

      if (diseno === 'propuesto') {
        const qRanura = aberturaQ(h, P.b, P.ranuraH, P.cd);
        const qVent = aberturaQ(h - P.reboseH, P.ventanaW, 8, P.cd);
        qsal = qRanura + qVent;
        perdidoRebose += qVent * dt;
        if (i % pasoSensor === 0) {
          const hw = h * P.rhoReal + P.ruido * gauss();   // lo que "ve" el sensor de presión
          const r = fw.muestra(hw, t, pasoSensor * dt);
          qMedActual = r.q; estadoActual = r.estado; volActual = r.vol;
        }
        qmedInst = qMedActual;
      } else if (diseno === 'original') {
        // Sensor de microflujo: canal de ~2,5 mm y saturación a 150 mL/min (2,5 mL/s)
        qsal = orificioQ(h + 20, 2.5, 0.62);
        qmedInst = Math.min(qsal, 2.5);
      } else {
        // Turbina 1/2": paso efectivo ~7 mm; el rotor no gira por debajo de 1 L/min
        qsal = orificioQ(h + 20, 7, 0.62);
        qmedInst = qsal >= 16.7 ? qsal : 0;
      }

      qsal = Math.min(qsal, S / dt + qin);
      if (qsal > qSalMax) qSalMax = qsal;
      S += (qin - qsal) * dt;
      if (S < 0) S = 0;
      let derrInst = 0;
      if (S > Smax) {
        derrInst = (S - Smax) / dt;
        derramado += S - Smax;
        if (tDesborde === null) tDesborde = t;
        S = Smax;
      }
      hMax = Math.max(hMax, h);
      qmaxReal.add(qin, dt);

      if (diseno !== 'propuesto') {
        vMed += qmedInst * dt;
        qmaxMed.add(qmedInst, dt);
        volActual = vMed;
        estadoActual = t > flujo.t0 && t < flujo.t0 + flujo.T + 6 ? 'MIDIENDO' : (t <= flujo.t0 ? 'LISTO' : 'RESULTADO');
      }

      if (i % pasoSerie === 0) {
        serie.t.push(t);
        serie.qin.push(qin);
        serie.qmed.push(qmedInst);
        serie.qsal.push(qsal);
        serie.derr.push(derrInst);
        serie.nivel.push(h);
        serie.estado.push(estadoActual);
        serie.vol.push(volActual);
      }
    }

    let res;
    if (diseno === 'propuesto') {
      const r = fw.resultado || { vol: fw.vol, qmax: 0, qmed: 0, tMiccion: 0, rebose: false, aviso: false };
      res = { vMed: r.vol, qmaxMed: r.qmax, qmedMed: r.qmed, tMiccion: r.tMiccion, rebose: r.rebose, aviso: r.aviso };
    } else {
      res = { vMed, qmaxMed: qmaxMed.max, qmedMed: 0, tMiccion: flujo.T, rebose: false, aviso: false };
    }

    return Object.assign(res, {
      diseno, perfil, vReal: vol,
      errPct: (res.vMed - vol) / vol * 100,
      qmaxReal: qmaxReal.max,
      hMax, limite, nivelAviso, nivelRebose, unidadNivel,
      margen: limite - hMax,
      derramado, perdidoRebose, tDesborde, qSalMax,
      duracion: tFin, tInicio: flujo.t0, tFinFlujo: flujo.t0 + flujo.T,
      serie,
    });
  }

  function simularCompacto(perfil, vol, qmax, Pin) {
    // Del exterior solo se aceptan la postura y el sensor: la geometría es la de la v0.2
    const P = Object.assign({}, POR_DEFECTO, COMPACTO);
    ['incl', 'vaiven', 'correccion', 'errCal', 'imuErr', 'rhoReal', 'rhoFw', 'ruido'].forEach(k => {
      if (Pin && Pin[k] !== undefined) P[k] = Pin[k];
    });
    const dt = 0.005;
    const flujo = generarFlujo(perfil, vol, qmax, dt, 11);
    const pasoSensor = Math.round(1 / 40 / dt);
    const pasoSerie = Math.round(0.05 / dt);
    const rng = mulberry32(99);
    const gauss = () => {
      const u = Math.max(1e-12, rng()), v = rng();
      return Math.sqrt(-2 * Math.log(u)) * Math.cos(2 * Math.PI * v);
    };
    const geo = { nRan: P.nRan, r: P.r, rT: P.rT, phiT: P.phiT, fase: 0 };
    const aReal = coefRanura(P.nRan * P.b, P.cd);
    const fw = crearFirmware({
      a: aReal * (1 + P.errCal / 100), n: 1.5, rho: P.rhoFw, geo, corregir: P.correccion,
      hAviso: P.ranuraH - 4, hRebose: P.reboseH,
    });
    const A = (Math.PI / 4 * P.camD * P.camD - Math.PI / 4 * (P.buzoExt ** 2 - P.buzoInt ** 2)) / 1000; // mL/mm
    const Vcam = A * P.camH, Smax = Vcam + P.embudoV, embudoH = 50;
    const rotX = P.imuErr * Math.PI / 180;
    const [ct, st] = eje(P.phiT);

    const serie = { t: [], qin: [], qmed: [], qsal: [], nivel: [], estado: [], vol: [], derr: [], incl: [] };
    const qmaxReal = crearQmax();
    let S = 0, derramado = 0, perdidoRebose = 0, hMax = 0, tDesborde = null, qSalMax = 0, tFin = 0;
    let qMedActual = 0, estadoActual = 'LISTO', volActual = 0;

    for (let i = 0; i < flujo.n; i++) {
      const t = i * dt;
      if (t > flujo.t0 + flujo.T + 12 && fw.estado !== 'MIDIENDO') break;
      tFin = t;
      // Postura: inclinación media + balanceo de la mano y pequeños golpes verticales
      const th = Math.max(0, P.incl + P.vaiven * Math.sin(2 * Math.PI * 0.6 * t) + 0.4 * P.vaiven * Math.sin(2 * Math.PI * 1.7 * t + 1)) * Math.PI / 180;
      const psi = (25 + (P.vaiven > 0 ? 30 * Math.sin(2 * Math.PI * 0.2 * t) : 0)) * Math.PI / 180;
      const up = [Math.sin(th) * Math.cos(psi), Math.sin(th) * Math.sin(psi), Math.cos(th)];
      const g = 1 + 0.004 * P.vaiven * Math.sin(2 * Math.PI * 1.1 * t + 0.5);

      const qin = flujo.q[i];
      const hAx = S / A;                         // altura del líquido en el eje
      const dc = hAx * up[2];
      let qRan = 0, qVent = 0;
      for (let k = 0; k < P.nRan; k++) {
        const [c, s] = eje(k * 2 * Math.PI / P.nRan);
        const di = dc - P.r * (up[0] * c + up[1] * s);
        const tanA = (-up[0] * s + up[1] * c) / up[2];
        qRan += aberturaQ(di, P.b * Math.sqrt(1 + tanA * tanA), P.ranuraH * up[2], P.cd) * Math.sqrt(g);
      }
      for (let k = 0; k < P.nVent; k++) {
        const [c, s] = eje(Math.PI / 4 + k * 2 * Math.PI / P.nVent);
        const dw = dc - P.r * (up[0] * c + up[1] * s) - P.reboseH * up[2];
        qVent += aberturaQ(dw, P.ventanaW, 5 * up[2], P.cd) * Math.sqrt(g);
      }
      const qsal = Math.min(qRan + qVent, S / dt + qin);
      if (qsal > qSalMax) qSalMax = qsal;
      perdidoRebose += Math.min(qVent, qsal) * dt;
      S += (qin - qsal) * dt;
      if (S < 0) S = 0;
      let derrInst = 0;
      if (S > Smax) { derrInst = (S - Smax) / dt; derramado += S - Smax; if (tDesborde === null) tDesborde = t; S = Smax; }
      hMax = Math.max(hMax, hAx);
      qmaxReal.add(qin, dt);

      if (i % pasoSensor === 0) {
        const dT = dc - P.rT * (up[0] * ct + up[1] * st);
        const hw = P.rhoReal * g * Math.max(0, dT) + P.ruido * gauss();
        const f0 = up.map(v => v * g);
        const imu = [f0[0] + 0.002 * gauss(),
                     f0[1] * Math.cos(rotX) - f0[2] * Math.sin(rotX) + 0.002 * gauss(),
                     f0[1] * Math.sin(rotX) + f0[2] * Math.cos(rotX) + 0.002 * gauss()];
        const r = fw.muestra(hw, t, pasoSensor * dt, imu);
        qMedActual = r.q; estadoActual = r.estado; volActual = r.vol;
      }

      if (i % pasoSerie === 0) {
        serie.t.push(t); serie.qin.push(qin); serie.qmed.push(qMedActual); serie.qsal.push(qsal);
        serie.nivel.push(Math.min(hAx, P.camH + embudoH)); serie.estado.push(estadoActual);
        serie.vol.push(volActual); serie.derr.push(derrInst); serie.incl.push(th * 180 / Math.PI);
      }
    }

    const r = fw.resultado || { vol: fw.vol, qmax: 0, qmed: 0, tMiccion: 0, rebose: false, aviso: false, inclMax: 0, inclinado: false };
    return {
      diseno: 'compacto', perfil, vReal: vol, vMed: r.vol,
      errPct: (r.vol - vol) / vol * 100, errMl: r.vol - vol,
      qmaxMed: r.qmax, qmedMed: r.qmed, tMiccion: r.tMiccion, rebose: r.rebose, aviso: r.aviso,
      inclMax: r.inclMax, inclinado: r.inclinado,
      qmaxReal: qmaxReal.max, hMax, limite: P.camH + embudoH, nivelAviso: P.ranuraH, nivelRebose: P.reboseH,
      unidadNivel: 'mm', margen: P.camH + embudoH - hMax, derramado, perdidoRebose, tDesborde, qSalMax,
      duracion: tFin, tInicio: flujo.t0, tFinFlujo: flujo.t0 + flujo.T, serie, geo: P,
    };
  }

  const api = { simular, caudalModelo, COMPACTO, generarFlujo, aberturaQ, orificioQ, coefRanura, crearFirmware, crearQmax, PERFILES, DISENOS, POR_DEFECTO };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.Uro = api;
})(typeof window !== 'undefined' ? window : this);
