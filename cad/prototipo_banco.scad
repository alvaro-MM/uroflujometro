// ============================================================================
//  Prototipo de banco · piezas comunes a la v0.1 y la v0.2
//
//  No se abre solo: lo incluyen prototipo_v01_botella.scad y prototipo_v02_botella.scad,
//  que definen antes todas las variables que usa.
//
//  Idea: montar en casa, sin imprimir la cámara ni las ranuras. La cámara es un tubo
//  de botella de plástico (cortada por arriba y por abajo) con las ranuras cortadas a
//  mano. El tubo se pega en el asiento del suelo. El líquido NO se queda: entra por
//  arriba, sale por las ranuras, baja por la camisa y sale por la boquilla.
//
//    z = 0   : suelo de la cámara (también el borde inferior de las ranuras)
//    Ejes    : como el aparato real (ranura 1 hacia +X, electrónica hacia -Y)
// ============================================================================

$fn = 72;
e = 0.01;

R_tubo  = botella_d / 2;                  // radio exterior del tubo de botella
Rg0     = R_tubo - 1.4;                   // asiento del tubo (anillo hundido en el suelo)
Rg1     = R_tubo + 0.4;
Rd      = Rg1 + 3;                        // borde del suelo
Rc      = Rd + hueco;                     // radio interior de la camisa
Re      = Rc + pared;                     // radio exterior de la camisa
z_base  = -base_h;                        // cara inferior del suelo / arranque de la tolva
z_tolva = z_base - (Rc - salida_d / 2);   // tolva a 45°
z_pie   = z_tolva - salida_largo;
z_canal = -(base_h - 2.7);                // eje del canal de la toma
xe      = caja_ancho / 2 + pared_caja;    // semiancho exterior de la caja
y_caja0 = -(Re - 1);                      // cara trasera de la caja
y_cin   = y_caja0 - pared_caja;           // interior de la caja, atrás
y_cfin  = y_cin - caja_fondo;             // interior de la caja, delante
y_cext  = y_cfin - pared_caja;            // cara frontal

// Toma de presión: pozo en el suelo y canal hasta la caja. Los últimos 6 mm son de Ø3
// para pegar un tubito de latón de 3 mm (o una aguja roma) que lleva el tubo de silicona.
module canal_toma() {
  translate([toma_x, 0, z_canal]) cylinder(d = 3, h = -z_canal + e);
  translate([toma_x, 0, z_canal]) rotate([90, 0, 0]) cylinder(d = 2, h = -y_cin - 5);
  translate([toma_x, y_cin + 6, z_canal]) rotate([90, 0, 0]) cylinder(d = 3.1, h = 7);
}

// ------------------------------------------------------------------ cuerpo --
module cuerpo() {
  difference() {
    union() {
      translate([0, 0, z_base]) cylinder(r = Re, h = camisa_alto - z_base);             // camisa
      hull() {                                                                         // tolva
        translate([0, 0, z_base]) cylinder(r = Re, h = e);
        translate([0, 0, z_tolva - pared]) cylinder(d = salida_ext, h = e);
      }
      translate([0, 0, z_pie]) cylinder(d = salida_ext, h = z_tolva - z_pie + e);       // boquilla
      translate([-xe, y_cext, caja_z0]) cube([2 * xe, y_caja0 - y_cext, caja_z1 - caja_z0]);   // caja
      // relleno entre la caja plana y la camisa redonda (la camisa se vacía después)
      translate([-xe, y_caja0 - e, z_base]) cube([2 * xe, -y_caja0 - 10, min(caja_z1, camisa_alto) - z_base]);
    }
    translate([0, 0, z_base - e]) cylinder(r = Rc, h = camisa_alto - z_base + 1);        // interior de la camisa
    hull() {                                                                           // interior de la tolva
      translate([0, 0, z_base]) cylinder(r = Rc, h = e);
      translate([0, 0, z_tolva]) cylinder(d = salida_d, h = e);
    }
    translate([0, 0, z_pie - 1]) cylinder(d = salida_d, h = z_tolva - z_pie + 2);
    // interior de la caja, ventana de la OLED, botón y USB-C
    translate([-caja_ancho / 2, y_cfin, caja_z0 + pared_caja]) cube([caja_ancho, caja_fondo, caja_z1 - caja_z0]);
    translate([-oled_ancho / 2, y_cext - 1, caja_z1 - oled_dz]) cube([oled_ancho, pared_caja + 2, oled_alto]);
    translate([0, y_cfin + 1, caja_z1 - boton_dz]) rotate([90, 0, 0]) cylinder(d = 7, h = pared_caja + 2);
    if (usb == "suelo")
      translate([-5, (y_cin + y_cfin) / 2 - 2.25, caja_z0 - 1]) cube([10, 4.5, pared_caja + 2]);
    else
      translate([-xe - 1, y_cfin + caja_fondo / 2 - 5, caja_z0 + 14]) cube([pared_caja + 2, 10, 4.5]);
    canal_toma();
  }
  // Suelo de la cámara con el asiento del tubo, sujeto a la camisa por 6 nervios que
  // bajan hasta la tolva (así se imprime de pie sin soportes). El del lado de la caja
  // lleva dentro el canal de la toma.
  difference() {
    union() {
      rotate_extrude() polygon([[0, z_base], [Rd, z_base], [Rd, -3], [Rg1, 0], [0, 0]]);
      for (a = [30, 90, 150, 210, 270, 330]) rotate([0, 0, a]) rotate([90, 0, 0])
        linear_extrude(height = 2.4, center = true)
          polygon([[salida_d / 2 + 3, z_base + 1], [Rc + 1, z_base + 1], [Rc + 1, z_base - 1.5],
                   [salida_d / 2 + 3 + 0.5, z_base - (Rc - salida_d / 2 - 3.5) - 1.5]]);
      translate([toma_x - 2.5, -(Rc + 1.5), z_base]) cube([5, Rc + 1.5 - (Rd - 3), base_h - 1]);   // nervio de la toma
    }
    rotate_extrude() translate([Rg0, -asiento]) square([Rg1 - Rg0, asiento + e]);       // asiento del tubo
    canal_toma();
  }
}

// ---------------------------------------------------------- tapa de la caja --
module caja_tapa() {
  difference() {
    union() {
      translate([-xe, y_cext, 0]) cube([2 * xe, (y_cin + 0.5) - y_cext, 2]);
      translate([-caja_ancho / 2 + 0.3, y_cfin + 0.3, -tapon_h]) cube([caja_ancho - 0.6, caja_fondo - 0.6, tapon_h + e]);
    }
    translate([-caja_ancho / 2 + 1.9, y_cfin + 1.9, -tapon_h - 1]) cube([caja_ancho - 3.8, caja_fondo - 3.8, tapon_h + 1]);
    if (muesca) translate([0, y_cext - 1, 1]) rotate([-90, 0, 0]) cylinder(d = 10, h = 6);   // muesca para abrir
  }
}

// ---------------------------------------------------------------- deflector --
// Tapa sobre la toma de presión: el chorro cae encima y se reparte hacia los lados, así
// no golpea el pozo (hace el papel del tapón del buzo). Se apoya en el suelo sin pegar.
module deflector() {
  difference() {
    union() {
      translate([0, 0, 5]) cylinder(d = 24, h = 1.6);
      for (a = [90, 210, 330]) rotate([0, 0, a]) translate([8, 0, 0]) cylinder(d = 3, h = 5.5);
    }
  }
}

// ---------------------------------------------------------------- plantilla --
// Anillo que se pone sobre el tubo de botella para marcar las ranuras. Las ranuras
// quedan a n_ranuras igualmente repartidas, la 1 hacia la muesca del borde superior.
module plantilla() {
  Ri = R_tubo + 0.25;
  difference() {
    cylinder(r = Ri + 2.2, h = tubo_largo);
    translate([0, 0, -1]) cylinder(r = Ri, h = tubo_largo + 2);
    for (i = [0 : n_ranuras - 1]) rotate([0, 0, i * 360 / n_ranuras])
      translate([Ri - 1, -(ranura_b + 0.2) / 2, asiento]) cube([4.5, ranura_b + 0.2, ranura_h]);
    translate([Ri - 1, -1.5, tubo_largo - 3]) cube([4.5, 3, 4]);     // marca de la ranura 1
  }
}

// ------------------------------------------------------------------ montaje --
module montaje() {
  color("dimgray") cuerpo();
  color("dimgray") translate([0, 0, caja_z1]) caja_tapa();
  color("orange") translate([toma_x, 0, 0]) deflector();
  color("skyblue", 0.35) difference() {                       // tubo de botella con sus ranuras
    translate([0, 0, -asiento]) cylinder(r = R_tubo, h = tubo_largo);
    translate([0, 0, -asiento - 1]) cylinder(r = R_tubo - 0.4, h = tubo_largo + 2);
    for (i = [0 : n_ranuras - 1]) rotate([0, 0, i * 360 / n_ranuras])
      translate([R_tubo - 2, -ranura_b / 2, 0]) cube([4, ranura_b, ranura_h]);
  }
}

if (pieza == "todo")      montaje();
if (pieza == "cuerpo")    cuerpo();                           // se imprime de pie, tal cual
if (pieza == "caja_tapa") rotate([180, 0, 0]) caja_tapa();    // placa abajo
if (pieza == "deflector") deflector();
if (pieza == "plantilla") plantilla();
