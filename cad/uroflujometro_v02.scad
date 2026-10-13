// ============================================================================
//  Uroflujómetro portátil v0.2 · OpenSCAD
//
//  Cámara de Ø24 mm con 4 ranuras verticales de 1,5 mm repartidas alrededor.
//  Lo que sale por las ranuras cae por una camisa anular a una boquilla central.
//  La toma de presión está en el centro del suelo, protegida bajo el tapón del
//  buzo, que reparte la entrada por 4 ventanas laterales orientadas entre ranuras.
//
//  Piezas: cuerpo, tapa, buzo, caja_tapa y embudo acodado.
//  Para exportar: elige "pieza", pulsa F6 y luego "Exportar como STL".
//  Material: PETG (el embudo puede ir en TPU para que sea flexible).
//
//  Ejes: Z hacia arriba; ranura 1 hacia +X, ranura 2 hacia +Y; electrónica hacia -Y.
//  z = 0 es el suelo de la cámara.
// ============================================================================

pieza = "todo"; // [todo, cuerpo, tapa, buzo, caja_tapa, embudo]

/* [Cámara] */
cam_d  = 24;    // diámetro interior (debe coincidir con RADIO_RANURAS_MM x 2)
cam_h  = 48;    // altura interior desde el suelo
pared  = 2;
camisa = 4;     // hueco del anillo de descarga
base_h = 6;     // espesor del suelo (lleva el canal de la toma)

/* [Ranuras] */
n_ranuras    = 4;
ranura_b     = 1.5;   // debe coincidir con config.h
ranura_h     = 36;
rebose_h     = 40;
ventana_w    = 6;
ventana_alto = 5;

/* [Buzo] */
buzo_ext          = 14;
buzo_int          = 11;
buzo_hueco        = 6;   // separación entre el tapón y el suelo
buzo_ventana_w    = 7;
buzo_ventana_alto = 5;

/* [Salida] */
salida_d     = 10;
salida_ext   = 13;
tolva_h      = 14;
salida_largo = 8;

/* [Electrónica] */
caja_ancho = 40;    // interior en X (cabe la OLED de 0,91")
caja_fondo = 14;    // interior en Y
caja_suelo = -10;

/* [Embudo] */
embudo_boca = 75;
embudo_alto = 40;
codo        = 30;   // grados: permite usarlo también sentado
cuello_d    = 18;   // diámetro interior del cuello de la tapa

$fn = 72;
e = 0.01;

R       = cam_d / 2;       // 12: radio de las ranuras
Ri      = R + pared;       // 14
Rc      = Ri + camisa;     // 18
Re      = Rc + pared;      // 20
z_base  = -base_h;
z_tolva = z_base - tolva_h;
z_pie   = z_tolva - salida_largo;
y_caja0 = -(Re - 1);              // cara trasera de la caja (solapa con la camisa)
y_cin   = y_caja0 - pared;        // interior de la caja, atrás
y_cfin  = y_cin - caja_fondo;     // interior de la caja, delante
y_cext  = y_cfin - pared;         // cara frontal
z_canal = -3.5;
nervios = [30, 150, 270];         // el de 270° lleva el canal de la toma

// Toma en el centro del suelo y canal hasta la caja. Los últimos 6 mm son de Ø3 para
// pegar dentro un tubito de latón de 3 mm (o una aguja roma) donde va el tubo de silicona.
module canal_toma() {
  translate([0, 0, z_canal]) cylinder(d = 3, h = -z_canal + e);
  translate([0, 0, z_canal]) rotate([90, 0, 0]) cylinder(d = 2, h = -y_cin - 5);
  translate([0, y_cin + 6, z_canal]) rotate([90, 0, 0]) cylinder(d = 3.1, h = 6 + 1);
}

// ------------------------------------------------------------------ cuerpo --
module cuerpo() {
  // Camisa exterior, tolva, boquilla y caja de electrónica
  difference() {
    union() {
      translate([0, 0, z_base]) cylinder(r = Re, h = cam_h - z_base);
      hull() {
        translate([0, 0, z_base]) cylinder(r = Re, h = e);
        translate([0, 0, z_tolva - pared]) cylinder(d = salida_ext, h = e);
      }
      translate([0, 0, z_pie]) cylinder(d = salida_ext, h = z_tolva - z_pie + e);
      translate([-caja_ancho / 2 - pared, y_cext, caja_suelo])
        cube([caja_ancho + 2 * pared, y_caja0 - y_cext, cam_h - caja_suelo]);
    }
    translate([0, 0, z_base - e]) cylinder(r = Rc, h = cam_h - z_base + 1);
    hull() {
      translate([0, 0, z_base]) cylinder(r = Rc, h = e);
      translate([0, 0, z_tolva]) cylinder(d = salida_d, h = e);
    }
    translate([0, 0, z_pie - 1]) cylinder(d = salida_d, h = z_tolva - z_pie + 2);
    translate([-caja_ancho / 2, y_cfin, caja_suelo + 2]) cube([caja_ancho, caja_fondo, cam_h]);
    translate([-12, y_cext - 1, cam_h - 12]) cube([24, pared + 2, 8]);                    // ventana OLED
    translate([0, y_cfin + 1, cam_h - 24]) rotate([90, 0, 0]) cylinder(d = 7, h = pared + 2); // botón
    translate([-5, (y_cin + y_cfin) / 2 - 2.25, caja_suelo - 1]) cube([10, 4.5, pared + 2]);  // USB-C
    translate([-0.6, Re - 0.8, cam_h - 6]) cube([1.2, 2, 6 + e]);   // marca para orientar la tapa (+Y)
    canal_toma();
  }
  // Cámara: pared con ranuras y suelo, sujeta a la camisa por tres nervios
  difference() {
    union() {
      translate([0, 0, z_base]) cylinder(r = Ri, h = cam_h - z_base);
      // el nervio de 270° es más ancho porque lleva dentro el canal de Ø3
      for (a = nervios) let(w = a == 270 ? 5 : 2.4)
        rotate([0, 0, a]) translate([Ri - 1, -w / 2, z_base]) cube([camisa + 2, w, base_h - 1]);
    }
    cylinder(r = R, h = cam_h + 1);
    // ranuras: bajan 0,4 mm del suelo para que la cámara se vacíe del todo
    for (i = [0 : n_ranuras - 1])
      rotate([0, 0, i * 360 / n_ranuras]) translate([R - 1, -ranura_b / 2, -0.4]) cube([pared + 2, ranura_b, ranura_h + 0.4]);
    // ventanas de rebose interno, entre ranuras (además ventilan la cámara)
    for (i = [0 : n_ranuras - 1])
      rotate([0, 0, (i + 0.5) * 360 / n_ranuras]) translate([R - 1, -ventana_w / 2, rebose_h]) cube([pared + 2, ventana_w, ventana_alto]);
    canal_toma();
  }
}

// -------------------------------------------------------------------- tapa --
// Se imprime plana. El buzo entra desde arriba por el cuello y cuelga de su pestaña;
// una chaveta lo orienta para que sus ventanas queden entre las ranuras. La muesca del
// borde se alinea con la marca del cuerpo.
module tapa() {
  difference() {
    union() {
      cylinder(r = Re, h = 3);
      translate([0, 0, 3 - e]) cylinder(d = cuello_d + 4, h = 12);
      rotate([0, 0, 135]) translate([(Ri + Rc) / 2, 0, 3 - e]) cylinder(d = 6, h = 4);   // respiradero
    }
    translate([0, 0, -1]) cylinder(d = buzo_ext + 0.3, h = 5);
    rotate([0, 0, 45]) translate([6.5, -1.2, -1]) cube([2.2, 2.4, 3]);                    // ranura de la chaveta
    translate([0, 0, 1.5]) cylinder(d = buzo_ext + 3.4, h = 1.5 + e);                    // rebaje de la pestaña
    translate([0, 0, 3]) cylinder(d1 = buzo_ext + 3.4, d2 = cuello_d, h = 1);
    translate([0, 0, 4 - e]) cylinder(d = cuello_d, h = 12);
    rotate([0, 0, 135]) translate([(Ri + Rc) / 2, 0, -1]) cylinder(d = 2.5, h = 10);
    translate([-0.6, Re - 0.8, -1]) cube([1.2, 2, 5]);                                    // marca: alinéala con la del cuerpo
  }
}

// -------------------------------------------------------------------- buzo --
// Tubo con tapón cerrado abajo y 4 ventanas laterales: el chorro sale de lado y
// no golpea la toma de presión del centro.
module buzo() {
  largo = cam_h - buzo_hueco + 3;
  difference() {
    union() {
      cylinder(d = buzo_ext, h = largo);
      translate([0, 0, largo - 1.5]) cylinder(d = buzo_ext + 3, h = 1.5);
      rotate([0, 0, 45]) translate([6.5, -1, largo - 5.5]) cube([1.8, 2, 4]);   // chaveta
    }
    translate([0, 0, 1.5]) cylinder(d = buzo_int, h = largo);
    for (i = [0 : n_ranuras - 1])
      rotate([0, 0, (i + 0.5) * 360 / n_ranuras]) translate([0, -buzo_ventana_w / 2, 1.5]) cube([buzo_ext, buzo_ventana_w, buzo_ventana_alto]);
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

// ---------------------------------------------------------- embudo acodado --
// Caño recto que entra en el cuello, codo y boca cónica. Imprímelo con el caño
// abajo y soportes bajo la boca (o en TPU para que sea flexible).
module embudo() {
  g = 1.6; ext = cuello_d - 0.5; int = ext - 2 * g; Rb = 22; pasos = 6;
  function pos(a) = [-Rb + Rb * cos(a), 0, 15 + Rb * sin(a)];
  module disco(a, d) translate(pos(a)) rotate([0, -a, 0]) cylinder(d = d, h = 0.01, center = true);
  difference() {
    union() {
      cylinder(d = ext, h = 15);
      for (i = [0 : pasos - 1]) hull() { disco(i * codo / pasos, ext); disco((i + 1) * codo / pasos, ext); }
      translate(pos(codo)) rotate([0, -codo, 0]) cylinder(d1 = ext, d2 = embudo_boca + 2 * g, h = embudo_alto);
    }
    translate([0, 0, -1]) cylinder(d = int, h = 16 + e);
    for (i = [0 : pasos - 1]) hull() { disco(i * codo / pasos, int); disco((i + 1) * codo / pasos, int); }
    translate(pos(codo)) rotate([0, -codo, 0]) translate([0, 0, -e]) cylinder(d1 = int, d2 = embudo_boca, h = embudo_alto + 2 * e);
  }
}

// ------------------------------------------------------------------ montaje --
module montaje() {
  color("dimgray") cuerpo();
  color("silver", 0.85) translate([0, 0, cam_h]) tapa();
  color("lightgray") translate([0, 0, buzo_hueco]) buzo();
  color("dimgray") translate([0, 0, cam_h]) caja_tapa();
  color("white", 0.55) translate([0, 0, cam_h + 4]) embudo();
}

if (pieza == "todo")      montaje();
if (pieza == "cuerpo")    rotate([180, 0, 0]) cuerpo(); // boca abajo: el suelo de la cámara queda como puente
if (pieza == "tapa")      tapa();
if (pieza == "buzo")      rotate([180, 0, 0]) buzo();  // pestaña abajo
if (pieza == "caja_tapa") rotate([180, 0, 0]) caja_tapa();
if (pieza == "embudo")    embudo();
