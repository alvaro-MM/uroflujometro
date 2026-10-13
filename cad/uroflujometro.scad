// ============================================================================
//  Uroflujómetro de vertedero · prototipo v0.1 · OpenSCAD
//
//  Piezas: cuerpo (cámara + canal de descarga + compartimento de electrónica),
//  tapa, buzo (tubo de entrada), tapa de la caja, embudo y soporte para el inodoro.
//  Para exportar: elige "pieza", pulsa F6 y luego "Exportar como STL".
//  Material: PETG (aguanta mejor la humedad y la limpieza que el PLA).
//
//  Ejes: Z hacia arriba, ranura y descarga hacia +X, electrónica hacia -Y.
//  z = 0 es la cara inferior de la base de la cámara.
// ============================================================================

pieza = "todo"; // [todo, cuerpo, tapa, buzo, caja_tapa, embudo, soporte]

/* [Cámara de medida] */
cam_d   = 32;    // diámetro interior
cam_h   = 95;    // altura interior desde el suelo
pared   = 2.4;
base_h  = 10;    // espesor de la base (lleva el canal de la toma de presión)

/* [Vertedero] */
ranura_b     = 3;    // ancho de la ranura (debe coincidir con config.h)
ranura_h     = 70;   // alto de la ranura
rebose_h     = 78;   // altura de las ventanas de rebose interno
ventana_w    = 10;
ventana_alto = 8;

/* [Entrada] */
buzo_d      = 16;   // diámetro exterior del tubo de entrada
buzo_hueco  = 8;    // separación entre el buzo y el suelo
cuello_d    = 26;   // diámetro interior del encaje del embudo

/* [Descarga] */
desc_w       = 30;  // ancho interior del canal (Y)
desc_l       = 18;  // profundidad interior del canal (X)
salida_d     = 12;  // paso interior de la salida
salida_ext   = 16;  // exterior del espigón (tubo de 12 mm interior)
salida_largo = 22;
tolva_h      = 18;  // caída de la tolva bajo el suelo del canal

/* [Toma de presión] */
toma_d    = 4;
toma_x    = 12;     // junto a la ranura: el nivel se mide donde se vierte
espiga_d  = 4.5;    // para tubo de silicona de 3 mm interior

/* [Electrónica] */
caja_ancho = 46;    // interior en X
caja_fondo = 22;    // interior en Y

/* [Soporte] */
gancho_largo = 60;  // vuelo hasta el borde exterior del inodoro
gancho_caida = 22;

$fn = 72;
e = 0.01;

R       = cam_d / 2;
Re      = R + pared;
z_suelo = base_h;
z_top   = base_h + cam_h;
z_canal = base_h - 2;
z_tolva = z_canal - tolva_h;
z_pie   = z_tolva - salida_largo;      // apoyo en la mesa / cama de impresión
x_sal   = Re + desc_l / 2;
y_dext  = desc_w / 2 + pared;          // cara exterior del canal de descarga
y_cin   = -(y_dext + pared);           // cara interior del compartimento
y_cfin  = y_cin - caja_fondo;          // cara exterior interior del compartimento
x_buzo  = -5.5;
x_resp  = x_sal;

// ------------------------------------------------------------------ cuerpo --
module cuerpo() {
  difference() {
    union() {
      cylinder(r = Re, h = z_top);
      // canal de descarga
      translate([0, -y_dext, z_canal - pared])
        cube([Re + desc_l + pared, 2 * y_dext, z_top - z_canal + pared]);
      // tolva
      hull() {
        translate([0, -y_dext, z_canal - pared]) cube([Re + desc_l + pared, 2 * y_dext, e]);
        translate([x_sal, 0, z_tolva - pared]) cylinder(d = salida_ext, h = e);
      }
      // espigón de salida con un resalte para sujetar el tubo
      translate([x_sal, 0, z_pie]) {
        cylinder(d = salida_ext, h = salida_largo + e);
        translate([0, 0, 4]) cylinder(d1 = salida_ext, d2 = salida_ext + 1.6, h = 3);   // ancho hacia la base
      }
      // faldón bajo la cámara: permite imprimir sin soportes y dejarlo de pie
      translate([0, 0, z_pie]) cylinder(r = Re, h = -z_pie + e);
      // compartimento de electrónica
      translate([-caja_ancho / 2 - pared, y_cfin - pared, z_pie])
        cube([caja_ancho + 2 * pared, caja_fondo + 2 * pared, z_top - z_pie]);
      // espiga de la toma de presión (queda dentro del compartimento)
      translate([toma_x, -12, 5]) rotate([90, 0, 0]) {
        cylinder(d = espiga_d, h = -y_cin - 2);
        translate([0, 0, -y_cin - 6]) cylinder(d1 = espiga_d + 1, d2 = espiga_d, h = 2.5);   // ancho hacia la base
      }
    }

    // interior de la cámara
    translate([0, 0, z_suelo]) cylinder(r = R, h = cam_h + 1);
    // interior del canal de descarga (sin tocar la pared de la cámara)
    difference() {
      translate([0, -desc_w / 2, z_canal]) cube([Re + desc_l, desc_w, z_top - z_canal + 1]);
      translate([0, 0, -1]) cylinder(r = Re, h = z_top + 2);
    }
    hull() {
      translate([Re, -desc_w / 2, z_canal]) cube([desc_l, desc_w, e]);
      translate([x_sal, 0, z_tolva]) cylinder(d = salida_d, h = e);
    }
    translate([x_sal, 0, z_pie - 1]) cylinder(d = salida_d, h = salida_largo + 1 + e);
    // ranura: llega 0,6 mm por debajo del suelo para que la cámara se vacíe del todo
    translate([R - 1, -ranura_b / 2, z_suelo - 0.6]) cube([pared + 2, ranura_b, ranura_h + 0.6]);
    // ventanas de rebose interno (además ventilan la cámara)
    for (a = [-22, 22])
      rotate([0, 0, a]) translate([R - 1, -ventana_w / 2, z_suelo + rebose_h]) cube([pared + 2, ventana_w, ventana_alto]);
    // toma de presión: bajada vertical + canal horizontal hasta la espiga
    translate([toma_x, 0, 4]) cylinder(d = toma_d, h = z_suelo - 4 + e);
    translate([toma_x, 0, 5]) rotate([90, 0, 0]) cylinder(d = 2.5, h = -y_cin + 11);
    // interior del faldón (cono a 45° arriba: sin puentes)
    translate([0, 0, z_pie - 1]) cylinder(r = Re - 2, h = -z_pie - (Re - 2) - 1);
    translate([0, 0, -2 - (Re - 2)]) cylinder(r1 = Re - 2, r2 = 0, h = Re - 2);
    // interior del compartimento
    translate([-caja_ancho / 2, y_cfin, z_pie + 2]) cube([caja_ancho, caja_fondo, z_top - z_pie]);
    // ventana de la OLED y agujero del botón (cara frontal, -Y)
    translate([-12, y_cfin - pared - 1, z_top - 30]) cube([24, pared + 2, 13]);
    translate([0, y_cfin + 1, z_top - 44]) rotate([90, 0, 0]) cylinder(d = 7, h = pared + 2);
    // USB-C de carga (lateral -X, abajo)
    translate([-caja_ancho / 2 - pared - 1, y_cfin + caja_fondo / 2 - 5, z_pie + 14])
      cube([pared + 2, 10, 4.5]);
  }
}

// -------------------------------------------------------------------- tapa --
module huella_tapa() {
  circle(r = Re);
  translate([0, -y_dext]) square([Re + desc_l + pared, 2 * y_dext]);
}

// Se imprime tal cual, plana sobre la cama. El buzo entra desde arriba por el
// cuello y queda colgado de su pestaña en un rebaje de la placa; eso centra la tapa.
module tapa() {
  difference() {
    union() {
      linear_extrude(3) huella_tapa();
      translate([x_buzo, 0, 3 - e]) cylinder(d = cuello_d + 4, h = 18);   // cuello del embudo
      translate([x_resp, 6, 3 - e]) cylinder(d = 8, h = 6);               // respiradero
    }
    translate([x_buzo, 0, -1]) cylinder(d = buzo_d + 0.3, h = 5);          // paso del buzo
    translate([x_buzo, 0, 1.5]) cylinder(d = buzo_d + 4.4, h = 1.5 + e);   // rebaje de la pestaña
    translate([x_buzo, 0, 3]) cylinder(d1 = buzo_d + 4.4, d2 = cuello_d, h = 3); // transición
    translate([x_buzo, 0, 6 - e]) cylinder(d = cuello_d, h = 16);           // encaje del embudo
    translate([x_resp, 6, -1]) cylinder(d = 4, h = 12);
  }
}

// -------------------------------------------------------------------- buzo --
// Tubo con pestaña arriba; se imprime con la pestaña sobre la cama.
module buzo() {
  largo = cam_h - buzo_hueco + 3;     // la pestaña apoya a 1,5 mm sobre la base de la tapa
  difference() {
    union() {
      cylinder(d = buzo_d, h = largo);
      translate([0, 0, largo - 1.5]) cylinder(d = buzo_d + 4, h = 1.5);
    }
    translate([0, 0, -1]) cylinder(d = buzo_d - 3.2, h = largo + 2);
  }
}

// ---------------------------------------------------------- tapa de la caja --
module caja_tapa() {
  difference() {
    union() {
      translate([-caja_ancho / 2 - pared, y_cfin - pared, 0]) cube([caja_ancho + 2 * pared, caja_fondo + pared + 0.8, 2]);
      translate([-caja_ancho / 2 + 0.3, y_cfin + 0.3, -6]) cube([caja_ancho - 0.6, caja_fondo - 0.6, 6 + e]);
    }
    translate([-caja_ancho / 2 + 1.9, y_cfin + 1.9, -7]) cube([caja_ancho - 3.8, caja_fondo - 3.8, 7]);
    translate([0, y_cfin - pared - 1, 1]) rotate([-90, 0, 0]) cylinder(d = 10, h = 6);  // muesca para abrir
  }
}

// ------------------------------------------------------------------ embudo --
module embudo() {
  boca = 110; alto = 60; caño = cuello_d - 1; caño_l = 15; g = 1.8;
  difference() {
    union() {
      cylinder(d = caño, h = caño_l);
      translate([0, 0, caño_l]) cylinder(d1 = caño, d2 = boca, h = alto);
    }
    translate([0, 0, -1]) cylinder(d = caño - 2 * g, h = caño_l + 1 + e);
    translate([0, 0, caño_l]) cylinder(d1 = caño - 2 * g, d2 = boca - 2 * g, h = alto + e);
  }
}

// ----------------------------------------------------------------- soporte --
// Perfil en J que se imprime tumbado. Se sujeta al cuerpo con dos bridas
// que pasan por las ranuras y abrazan cámara y canal.
module soporte() {
  g = 6; ancho = 15; alto = 50;
  difference() {
    linear_extrude(ancho) union() {
      square([g, alto]);                                                  // apoyo contra la cámara
      translate([-gancho_largo, alto - 4]) square([gancho_largo + g, 4]); // brazo sobre el borde
      translate([-gancho_largo, alto - gancho_caida]) square([4, gancho_caida]); // caída exterior
      polygon([[0, alto - 4], [-14, alto - 4], [0, alto - 18]]);          // escuadra
    }
    for (y = [8, 26]) translate([1.9, y, -1]) cube([2.2, 5, ancho + 2]);  // túneles para bridas
  }
}

// ------------------------------------------------------------------ montaje --
module montaje() {
  color("dimgray") cuerpo();
  color("silver", 0.8) translate([0, 0, z_top]) tapa();
  color("lightgray") translate([x_buzo, 0, z_suelo + buzo_hueco]) buzo();   // pestaña en el rebaje
  color("dimgray") translate([0, 0, z_top]) caja_tapa();
  color("white", 0.5) translate([x_buzo, 0, z_top + 6]) embudo();
  color("orange") translate([-Re - 6, -7.5, z_top - 50]) rotate([90, 0, 0]) translate([0, 0, -15]) soporte();
}

if (pieza == "todo")      montaje();
if (pieza == "cuerpo")    cuerpo();
if (pieza == "tapa")      tapa();
if (pieza == "buzo")      rotate([180, 0, 0]) buzo();         // pestaña abajo
if (pieza == "caja_tapa") rotate([180, 0, 0]) caja_tapa();
if (pieza == "embudo")    embudo();
if (pieza == "soporte")   soporte();
