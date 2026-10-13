// ============================================================================
//  Prototipo de banco v0.2 · electrónica + botella · OpenSCAD
//
//  Sirve para montar y probar la electrónica de la v0.2 (XIAO ESP32-C3, MS4525DO,
//  LIS3DH, OLED de 0,91", botón y batería) sin imprimir la cámara, las ranuras ni
//  el buzo. En su lugar, una botella pequeña hace de depósito: la presión de su
//  fondo llega al sensor por un tubo de silicona, igual que en el aparato real.
//
//  Coincide con la v0.2 en lo que importa para la electrónica y el firmware:
//    · la caja tiene el mismo interior (40 × 14 mm) y las mismas aberturas;
//    · los ejes son los mismos (Z arriba, ranura 1 hacia +X, electrónica hacia -Y),
//      así que MAPA_ACC de config.h vale sin cambios;
//    · el eje de la botella ocupa el sitio del eje de la cámara.
//
//  Piezas: cuerpo (caja + anillo para la botella) y caja_tapa.
//  Para exportar: elige "pieza", pulsa F6 y luego "Exportar como STL". PETG o PLA.
//  z = 0 es la cara inferior del cuerpo.
// ============================================================================

pieza = "todo"; // [todo, cuerpo, caja_tapa]

/* [Botella] */
botella_d = 65;    // diámetro exterior de la botella: MÍDELO y ajústalo
holgura   = 0.6;   // juego entre botella y anillo
anillo_h  = 35;    // altura del anillo que sujeta la botella
suelo     = 3;     // espesor del fondo del anillo
tubo_z    = 8;     // altura del paso del tubo sobre el fondo del anillo (taladra la botella a esta altura)
tubo_d    = 6;     // paso del tubo de silicona de 3 mm interior

/* [Electrónica] (igual que cad/uroflujometro_v02.scad) */
caja_ancho = 40;   // interior en X (cabe la OLED de 0,91")
caja_fondo = 14;   // interior en Y
caja_alto  = 58;   // exterior en Z (como la caja del aparato real)
pared      = 2;

$fn = 72;
e = 0.01;

Rb      = botella_d / 2 + holgura;   // radio interior del anillo
Re      = Rb + pared;                // radio exterior del anillo
y_caja0 = -(Re - 1);                 // cara trasera de la caja (solapa 1 mm con el anillo)
y_cin   = y_caja0 - pared;           // interior de la caja, atrás
y_cfin  = y_cin - caja_fondo;        // interior de la caja, delante
y_cext  = y_cfin - pared;            // cara frontal

// ------------------------------------------------------------------ cuerpo --
module cuerpo() {
  difference() {
    union() {
      cylinder(r = Re, h = anillo_h);
      translate([-caja_ancho / 2 - pared, y_cext, 0])
        cube([caja_ancho + 2 * pared, y_caja0 - y_cext, caja_alto]);
    }
    // hueco de la botella
    translate([0, 0, suelo]) cylinder(r = Rb, h = anillo_h);
    // interior de la caja (abierta por arriba; la cierra caja_tapa)
    translate([-caja_ancho / 2, y_cfin, pared]) cube([caja_ancho, caja_fondo, caja_alto]);
    // ventana de la OLED y botón (cara frontal, -Y)
    translate([-12, y_cext - 1, caja_alto - 12]) cube([24, pared + 2, 8]);
    translate([0, y_cfin + 1, caja_alto - 24]) rotate([90, 0, 0]) cylinder(d = 7, h = pared + 2);
    // USB-C por el suelo
    translate([-5, (y_cin + y_cfin) / 2 - 2.25, -1]) cube([10, 4.5, pared + 2]);
    // paso del tubo de la toma de presión: de la caja al fondo de la botella
    translate([0, y_cin - 2, suelo + tubo_z]) rotate([-90, 0, 0]) cylinder(d = tubo_d, h = (y_cin - 2 < -Rb ? -Rb - (y_cin - 2) + 1 : 1));
  }
}

// ---------------------------------------------------------- tapa de la caja --
module caja_tapa() {
  difference() {
    union() {
      translate([-caja_ancho / 2 - pared, y_cext, 0]) cube([caja_ancho + 2 * pared, (y_cin + 0.5) - y_cext, 2]);
      translate([-caja_ancho / 2 + 0.3, y_cfin + 0.3, -5]) cube([caja_ancho - 0.6, caja_fondo - 0.6, 5 + e]);
    }
    translate([-caja_ancho / 2 + 1.9, y_cfin + 1.9, -6]) cube([caja_ancho - 3.8, caja_fondo - 3.8, 6]);
  }
}

// ------------------------------------------------------------------ montaje --
module montaje() {
  color("dimgray") cuerpo();
  color("dimgray") translate([0, 0, caja_alto]) caja_tapa();
  color("skyblue", 0.35) translate([0, 0, suelo]) cylinder(d = botella_d, h = 90);   // botella (solo para verla)
}

if (pieza == "todo")      montaje();
if (pieza == "cuerpo")    cuerpo();
if (pieza == "caja_tapa") rotate([180, 0, 0]) caja_tapa();   // placa abajo
