// Modelos 3D (three.js) de cada versión, con la misma geometría que los .scad.
// Unidades en mm. Ejes del .scad: Z arriba, electrónica hacia -Y. En three.js: Y arriba
// y la electrónica hacia +Z (mirando a la cámara).
(function (root) {
  'use strict';

  function crear(THREE, id) {
    const todos = [];
    const mat = (color, extra) => {
      const m = new THREE.MeshStandardMaterial(Object.assign({ color, roughness: 0.6, metalness: 0.05, side: THREE.DoubleSide }, extra || {}));
      todos.push(m);
      return m;
    };
    const M = {
      cuerpo: mat(0x2b3036),
      tapa: mat(0x444b53),
      buzo: mat(0x6a747e),
      soporte: mat(0x5d6873),
      embudo: mat(0xe8eff5, { transparent: true, opacity: 0.5, depthWrite: false, roughness: 0.15 }),
      agua: mat(0xe3a52a, { transparent: true, opacity: 0.62, depthWrite: false, roughness: 0.3 }),
      boton: mat(0x1b1e22),
      xiao: mat(0x1f3a24),
      pcb: mat(0x1f4f9a),
      lis: mat(0x4a3394),
      lipo: mat(0xc9cfd6, { metalness: 0.4, roughness: 0.35 }),
      tubo: mat(0xd9d2c4, { roughness: 0.4 }),
      cuenco: mat(0x9db3c6, { roughness: 0.5 }),
      madera: mat(0xb98b55, { roughness: 0.8 }),
      taper: mat(0xeaf0f4, { transparent: true, opacity: 0.55, depthWrite: false, roughness: 0.2 }),
    };
    const V2 = (x, y) => new THREE.Vector2(x, y);
    const P = (x, y, z) => new THREE.Vector3(x, z, -y);       // del .scad a three.js
    const rad = d => d * Math.PI / 180;

    // Sólido de revolución: perfil [[r, z], ...] cerrado, de "desde" a "desde + ancho" grados del .scad
    function lathe(perfil, m, o) {
      o = o || {};
      const desde = o.desde || 0, ancho = o.ancho == null ? 360 : o.ancho;
      const g = new THREE.LatheGeometry(perfil.map(q => V2(q[0], q[1])), Math.max(4, Math.round(72 * ancho / 360)), rad(desde + 90), rad(ancho));
      const mesh = new THREE.Mesh(g, m);
      mesh.position.copy(P(o.cx || 0, o.cy || 0, 0));
      return mesh;
    }
    const anillo = (rIn, rOut, z0, z1, m, o) => lathe([[rIn, z0], [rOut, z0], [rOut, z1], [rIn, z1], [rIn, z0]], m, o);
    // Pared cilíndrica con huecos (ranuras o ventanas): [[ángulo central, ancho en grados], ...]
    function aroConHuecos(rIn, rOut, z0, z1, huecos, m, o) {
      const g = new THREE.Group();
      const h = huecos.map(([c, w]) => [c - w / 2, c + w / 2]).sort((a, b) => a[0] - b[0]);
      for (let i = 0; i < h.length; i++) {
        const ini = h[i][1], fin = i + 1 < h.length ? h[i + 1][0] : h[0][0] + 360;
        g.add(anillo(rIn, rOut, z0, z1, m, Object.assign({}, o, { desde: ini, ancho: fin - ini })));
      }
      return g;
    }
    function caja(x0, x1, y0, y1, z0, z1, m) {
      const mesh = new THREE.Mesh(new THREE.BoxGeometry(x1 - x0, z1 - z0, y1 - y0), m);
      mesh.position.copy(P((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2));
      return mesh;
    }
    // Caja abierta por arriba: e = exterior, i = interior, [x0, x1, y0, y1, z0, z1]
    function cajaHueca(e, i, m) {
      const g = new THREE.Group();
      g.add(caja(e[0], e[1], e[2], i[2], e[4], e[5], m));
      g.add(caja(e[0], e[1], i[3], e[3], e[4], e[5], m));
      g.add(caja(e[0], i[0], i[2], i[3], e[4], e[5], m));
      g.add(caja(i[1], e[1], i[2], i[3], e[4], e[5], m));
      g.add(caja(i[0], i[1], i[2], i[3], e[4], i[4], m));
      return g;
    }
    // Nervio radial a un ángulo del .scad
    function radial(phi, r0, r1, w, z0, z1, m) {
      const mesh = new THREE.Mesh(new THREE.BoxGeometry(r1 - r0, z1 - z0, w), m);
      mesh.position.set((r0 + r1) / 2, (z0 + z1) / 2, 0);
      const piv = new THREE.Group();
      piv.add(mesh);
      piv.rotation.y = rad(phi);
      return piv;
    }
    // Superficie entre dos contornos cerrados con el mismo número de puntos
    function loft(sup, inf, m) {
      const pos = [];
      for (let i = 0; i < sup.length; i++) {
        const j = (i + 1) % sup.length;
        [sup[i], sup[j], inf[j], sup[i], inf[j], inf[i]].forEach(v => pos.push(v.x, v.y, v.z));
      }
      const g = new THREE.BufferGeometry();
      g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
      g.computeVertexNormals();
      return new THREE.Mesh(g, m);
    }
    function pantalla(ancho, alto, texto, sub) {
      const c = document.createElement('canvas');
      c.width = 512; c.height = Math.round(512 * alto / ancho);
      const x = c.getContext('2d');
      x.fillStyle = '#03070b'; x.fillRect(0, 0, c.width, c.height);
      x.fillStyle = '#cfefff'; x.textBaseline = 'middle';
      x.font = `600 ${Math.round(c.height * 0.62)}px "Segoe UI", system-ui, sans-serif`;
      x.fillText(texto, c.width * 0.06, c.height * 0.54);
      const w = x.measureText(texto).width;
      x.font = `${Math.round(c.height * 0.26)}px "Segoe UI", system-ui, sans-serif`;
      x.fillText('mL', c.width * 0.06 + w + 12, c.height * 0.62);
      if (sub) { x.textAlign = 'right'; x.fillText(sub, c.width * 0.95, c.height * 0.62); }
      const m = new THREE.MeshBasicMaterial({ map: new THREE.CanvasTexture(c), side: THREE.DoubleSide });
      todos.push(m);
      return new THREE.Mesh(new THREE.PlaneGeometry(ancho, alto), m);
    }
    function botonFrente(x, y, z) {
      const b = new THREE.Mesh(new THREE.CylinderGeometry(3.5, 3.5, 2, 32), M.boton);
      b.rotation.x = Math.PI / 2;
      b.position.copy(P(x, y - 1, z));
      return b;
    }
    const grupo = (...hijos) => { const g = new THREE.Group(); hijos.forEach(h => g.add(h)); return g; };
    const V3 = (x, y, z) => new THREE.Vector3(x, y, z);

    // ------------------------------------------------------------ v0.1 --
    function v01() {
      const cuerpo = grupo(
        lathe([[0, 0], [18.4, 0], [18.4, 10], [0, 10], [0, 0]], M.cuerpo),
        lathe([[16.4, -32], [18.4, -32], [18.4, 0], [0, 0], [0, -2], [16.4, -18.4], [16.4, -32]], M.cuerpo),
        aroConHuecos(16, 18.4, 10, 80, [[0, 10]], M.cuerpo),                 // ranura de 3 mm
        anillo(16, 18.4, 80, 88, M.cuerpo),
        aroConHuecos(16, 18.4, 88, 96, [[22, 33], [-22, 33]], M.cuerpo),     // ventanas de rebose
        anillo(16, 18.4, 96, 105, M.cuerpo),
        caja(36.4, 38.8, -17.4, 17.4, 5.6, 105, M.cuerpo),                   // canal de descarga
        caja(10.6, 36.4, 15, 17.4, 5.6, 105, M.cuerpo),
        caja(10.6, 36.4, -17.4, -15, 5.6, 105, M.cuerpo),
        anillo(6, 8, -32, -12.4, M.cuerpo, { cx: 27.4 }),                     // espigón de salida
        anillo(6, 8.8, -28, -25, M.cuerpo, { cx: 27.4 }),
        cajaHueca([-25.4, 25.4, -44.2, -17.4, -32, 105], [-23, 23, -41.8, -19.8, -30, 105], M.cuerpo),
        botonFrente(0, -44.2, 61)
      );
      const sup = [], inf = [];                                               // tolva: rectángulo -> círculo
      for (let i = 0; i < 64; i++) {
        const a = i / 64 * Math.PI * 2, c = Math.cos(a), s = Math.sin(a);
        const t = Math.min(10.7 / Math.max(1e-6, Math.abs(c)), 17.4 / Math.max(1e-6, Math.abs(s)));
        sup.push(P(28.1 + t * c, t * s, 5.6));
        inf.push(P(27.4 + 8 * c, 8 * s, -12.4));
      }
      cuerpo.add(loft(sup, inf, M.cuerpo));
      const pan = pantalla(24, 13, '287');
      pan.position.copy(P(0, -44.3, 81.5));
      cuerpo.add(pan);

      const forma = new THREE.Shape();                                        // tapa: círculo + canal
      const a0 = Math.atan2(17.4, 5.98);
      forma.moveTo(5.98, -17.4); forma.lineTo(38.8, -17.4); forma.lineTo(38.8, 17.4); forma.lineTo(5.98, 17.4);
      forma.absarc(0, 0, 18.4, a0, Math.PI * 2 - a0, false);
      [[-5.5, 0, 8.15], [27.4, 6, 2]].forEach(([x, y, r]) => { const h = new THREE.Path(); h.absarc(x, y, r, 0, Math.PI * 2, true); forma.holes.push(h); });
      const gT = new THREE.ExtrudeGeometry(forma, { depth: 3, bevelEnabled: false, curveSegments: 48 });
      gT.rotateX(-Math.PI / 2);
      const placa = new THREE.Mesh(gT, M.tapa);
      placa.position.y = 105;
      const tapa = grupo(placa, anillo(13, 15, 108, 126, M.tapa, { cx: -5.5 }), anillo(2, 4, 108, 114, M.tapa, { cx: 27.4, cy: 6 }));

      const buzo = grupo(anillo(6.4, 8, 18, 108, M.buzo, { cx: -5.5 }), anillo(6.4, 10, 106.5, 108, M.buzo, { cx: -5.5 }));
      const embudo = grupo(lathe([[10.7, 111], [12.5, 111], [12.5, 126], [55, 186], [53.2, 186], [10.7, 126], [10.7, 111]], M.embudo, { cx: -5.5 }));
      const tapaCaja = grupo(caja(-25.4, 25.4, -44.2, -19, 105, 107, M.tapa));
      const soporte = grupo(caja(-24.4, -18.4, -7.5, 7.5, 55, 105, M.soporte), caja(-84.4, -18.4, -7.5, 7.5, 101, 105, M.soporte), caja(-84.4, -80.4, -7.5, 7.5, 83, 105, M.soporte));
      const electronica = grupo(
        caja(-10.5, 10.5, -24, -22, -24, -6, M.xiao), caja(-17, 17, -40, -35, -26, 4, M.lipo),
        caja(4, 20, -26, -22, 88, 100, M.pcb), caja(-13.7, 13.7, -41.6, -39.6, 68, 96, M.pcb));
      const agua = grupo(
        lathe([[0, 10], [15.9, 10], [15.9, 40], [0, 40], [0, 10]], M.agua),
        caja(18.8, 22, -1.5, 1.5, 6, 40, M.agua),
        anillo(0, 3, -70, -12, M.agua, { cx: 27.4 }));

      return montar([
        { nombre: 'Cuerpo', obj: cuerpo, despiece: V3(0, 0, 0) },
        { nombre: 'Soporte', obj: soporte, despiece: V3(-45, 0, 0) },
        { nombre: 'Electrónica', obj: electronica, despiece: V3(0, 150, 45) },
        { nombre: 'Tapa de la caja', obj: tapaCaja, despiece: V3(0, 45, 30) },
        { nombre: 'Tapa', obj: tapa, despiece: V3(0, 60, 0) },
        { nombre: 'Buzo', obj: buzo, despiece: V3(0, 150, 0) },
        { nombre: 'Embudo', obj: embudo, despiece: V3(0, 235, 0) },
      ], agua, [-32, 186]);
    }

    // ------------------------------------------------------------ v0.2 --
    function v02() {
      const dr = 1.5 / 13 * 180 / Math.PI, dv = 6 / 13 * 180 / Math.PI;
      const cuerpo = grupo(
        anillo(18, 20, -6, 48, M.cuerpo),                                                     // camisa
        lathe([[18, -6], [20, -6], [6.5, -22], [6.5, -28], [5, -28], [5, -20], [18, -6]], M.cuerpo), // tolva y boquilla
        lathe([[0, -6], [14, -6], [14, 0], [0, 0], [0, -6]], M.cuerpo),                       // suelo
        aroConHuecos(12, 14, 0, 36, [0, 90, 180, 270].map(a => [a, dr]), M.cuerpo),          // 4 ranuras
        anillo(12, 14, 36, 40, M.cuerpo),
        aroConHuecos(12, 14, 40, 45, [45, 135, 225, 315].map(a => [a, dv]), M.cuerpo),       // rebose
        anillo(12, 14, 45, 48, M.cuerpo),
        cajaHueca([-22, 22, -37, -19, -10, 48], [-20, 20, -35, -21, -8, 48], M.cuerpo),
        botonFrente(0, -37, 24)
      );
      [30, 150, 270].forEach(a => cuerpo.add(radial(a, 13, 19, a === 270 ? 5 : 2.4, -6, -1, M.cuerpo)));
      const pan = pantalla(24, 8, '287', 'apuntar');
      pan.position.copy(P(0, -37.1, 40));
      cuerpo.add(pan);

      const c135 = [16 * Math.cos(rad(135)), 16 * Math.sin(rad(135))];
      const tapa = grupo(anillo(7.15, 20, 48, 51, M.tapa), anillo(9, 11, 51, 63, M.tapa), anillo(1.25, 3, 51, 55, M.tapa, { cx: c135[0], cy: c135[1] }));
      const buzo = grupo(
        aroConHuecos(5.5, 7, 7.5, 12.5, [45, 135, 225, 315].map(a => [a, 64]), M.buzo),     // ventanas laterales
        anillo(5.5, 7, 12.5, 51, M.buzo),
        lathe([[0, 6], [7, 6], [7, 7.5], [0, 7.5], [0, 6]], M.buzo),                          // tapón
        anillo(5.5, 8.5, 49.5, 51, M.buzo));                                                  // pestaña

      // Embudo acodado: gira libre en el cuello; aquí con el codo hacia delante-izquierda
      const bd = [-Math.SQRT1_2, -Math.SQRT1_2], z0 = 67, RB = 22, codo = 30;
      const p3 = a => { const k = RB * (1 - Math.cos(rad(a))); return P(bd[0] * k, bd[1] * k, z0 + RB * Math.sin(rad(a))); };
      const h3 = a => P(bd[0] * Math.sin(rad(a)), bd[1] * Math.sin(rad(a)), Math.cos(rad(a))).normalize();
      class Arco extends THREE.Curve { getPoint(t, out) { return (out || new THREE.Vector3()).copy(p3(t * codo)); } }
      const cono = lathe([[8.75, 0], [39, 40], [37.4, 40], [7.15, 0], [8.75, 0]], M.embudo);
      cono.quaternion.setFromUnitVectors(V3(0, 1, 0), h3(codo));
      cono.position.copy(p3(codo));
      const embudo = grupo(anillo(7.25, 8.75, 52, 67, M.embudo), new THREE.Mesh(new THREE.TubeGeometry(new Arco(), 12, 8.75, 32, false), M.embudo), cono);

      const tapaCaja = grupo(caja(-22, 22, -37, -20.5, 48, 50, M.tapa));
      const electronica = grupo(
        caja(-10.5, 10.5, -25, -23, -6, 12, M.xiao), caja(-15, 15, -34.5, -30, -6, 16, M.lipo),
        caja(-18, -4, -26, -22, 26, 40, M.pcb), caja(6, 18, -34.6, -33.4, 18, 30, M.lis),
        caja(-19, 19, -34.8, -33, 32, 46, M.pcb));
      const agua = grupo(lathe([[0, 0], [12, 0], [12, 18], [0, 18], [0, 0]], M.agua), anillo(0, 3, -70, -28, M.agua));

      return montar([
        { nombre: 'Cuerpo', obj: cuerpo, despiece: V3(0, 0, 0) },
        { nombre: 'Electrónica', obj: electronica, despiece: V3(0, 100, 45) },
        { nombre: 'Tapa de la caja', obj: tapaCaja, despiece: V3(0, 32, 22) },
        { nombre: 'Tapa', obj: tapa, despiece: V3(0, 45, 0) },
        { nombre: 'Buzo', obj: buzo, despiece: V3(0, 100, 0) },
        { nombre: 'Embudo', obj: embudo, despiece: V3(0, 178, 0) },
      ], agua, [-28, 132]);
    }

    // ------------------------------------------- prototipos caseros (sin impresión) --
    // Cámara: vaso o botella de fondo plano cortada, con las ranuras cortadas a mano. Se apoya
    // en dos cucharas de madera sobre un cuenco que recoge lo que sale. La electrónica va en
    // un táper. Ver el apartado "Prototipo casero" del README.
    function casero(v) {
      const R = 32.5, H = v === 'v01' ? 95 : 60;
      const n = v === 'v01' ? 1 : 4, rb = v === 'v01' ? 3 : 1.5, rh = v === 'v01' ? 70 : 36, tomaX = v === 'v01' ? 12 : 0;
      const zFondo = -62, yTf = -160, yTb = -95;                         // táper: cara frontal y trasera
      const varilla = (a, b, r, m) => {
        const A = P(a[0], a[1], a[2]), B = P(b[0], b[1], b[2]), d = B.clone().sub(A);
        const mesh = new THREE.Mesh(new THREE.CylinderGeometry(r, r, d.length(), 12), m);
        mesh.position.copy(A.clone().add(B).multiplyScalar(0.5));
        mesh.quaternion.setFromUnitVectors(V3(0, 1, 0), d.normalize());
        return mesh;
      };

      const hb = rb / R * 180 / Math.PI;
      const camara = grupo(
        lathe([[0, -1.5], [R, -1.5], [R, 0], [0, 0], [0, -1.5]], M.embudo),
        aroConHuecos(R - 0.5, R, 0, rh, Array.from({ length: n }, (_, i) => [i * 360 / n, hb]), M.embudo),
        anillo(R - 0.5, R, rh, H, M.embudo));
      const soporte = grupo(
        lathe([[0, -62], [50, -62], [80, -8], [78, -8], [49, -60], [0, -60], [0, -62]], M.cuenco),
        caja(-100, 100, -30, -18, -9, -1.5, M.madera), caja(-100, 100, 18, 30, -9, -1.5, M.madera));
      const deflector = grupo(lathe([[0, 5], [12, 5], [12, 6.6], [0, 6.6], [0, 5]], M.boton, { cx: tomaX }));
      [90, 270].forEach(a => deflector.add(anillo(0, 1.5, 0, 5.5, M.boton, { cx: tomaX + 8 * Math.cos(rad(a)), cy: 8 * Math.sin(rad(a)) })));
      const embudo = grupo(lathe([[3, H - 36], [36, H + 4], [37, H + 4], [4, H - 36], [3, H - 36]], M.embudo));
      const tubo = grupo(
        varilla([0, -120, zFondo + 3], [0, -96, -45], 1.8, M.tubo),
        varilla([0, -96, -45], [0, -82, -5], 1.8, M.tubo),
        varilla([0, -82, -5], [0, -R, 3], 1.8, M.tubo),
        varilla([0, -R, 3], [tomaX, 0, 3], 1.8, M.tubo));

      const taper = grupo(
        cajaHueca([-55, 55, yTf, yTb, zFondo, -4], [-53, 53, yTf + 2, yTb - 2, zFondo + 2, -4], M.taper),
        caja(-55, 55, yTf, yTb, -4, -2, M.taper),
        botonFrente(38, yTf, -30));
      const pan = pantalla(24, v === 'v01' ? 13 : 8, '287', v === 'v02' ? 'apuntar' : '');
      pan.position.copy(P(0, yTf - 0.1, -30));
      taper.add(pan);
      const zp = zFondo + 2;
      const electronica = grupo(
        caja(-42, -21, -134, -116.5, zp, zp + 3, M.xiao), caja(-10, 10, -136, -116, zp, zp + 4, M.pcb),
        v === 'v02' ? caja(16, 36, -136, -116, zp, zp + 1.6, M.lis) : caja(16, 36, -136, -116, zp, zp + 1.6, M.pcb),
        caja(-14, 14, yTf + 2, yTf + 4, -40, -20, M.pcb));
      const agua = grupo(
        lathe([[0, 0], [R - 0.5, 0], [R - 0.5, 14], [0, 14], [0, 0]], M.agua),
        lathe([[0, -60], [44, -60], [44, -52], [0, -52], [0, -60]], M.agua));

      return montar([
        { nombre: 'Cuenco y cucharas', obj: soporte, despiece: V3(0, -45, 0) },
        { nombre: 'Cámara', obj: camara, despiece: V3(0, 55, 0) },
        { nombre: 'Tapón sobre la toma', obj: deflector, despiece: V3(0, 105, 0) },
        { nombre: 'Embudo', obj: embudo, despiece: V3(0, 165, 0) },
        { nombre: 'Táper', obj: taper, despiece: V3(0, 0, 60) },
        { nombre: 'Electrónica', obj: electronica, despiece: V3(0, 60, 60) },
        { nombre: 'Tubo de silicona', obj: tubo, despiece: V3(-90, 30, 0) },
      ], agua, [-62, H + 4]);
    }

    function montar(partes, agua, alto) {
      const g = new THREE.Group();
      partes.forEach(p => g.add(p.obj));
      g.add(agua);
      return { id, grupo: g, partes, agua, materiales: todos, alto };
    }

    if (id === 'v01b') return casero('v01');
    if (id === 'v02b') return casero('v02');
    return id === 'v01' ? v01() : v02();
  }

  // Separa las piezas (f de 0 a 1). El agua solo se ve con el aparato montado.
  function despiece(modelo, f, verAgua) {
    modelo.partes.forEach(p => { p.obj.position.copy(p.despiece).multiplyScalar(f); if (p.soloDespiece) p.obj.visible = f > 0.02; });
    modelo.agua.visible = !!verAgua && f < 0.02;
  }

  root.Modelos3D = { crear, despiece };
})(typeof window !== 'undefined' ? window : this);
