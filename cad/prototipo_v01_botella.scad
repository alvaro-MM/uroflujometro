// ============================================================================
//  Prototipo de banco v0.1 · electrónica + tubo de botella · OpenSCAD
//
//  Para montar en casa la v0.1 antes de tener el chasis definitivo. Lleva la misma
//  caja de electrónica (XIAO ESP32-C3, MS4525DO, OLED de 0,96", botón y batería),
//  pero la cámara de Ø32, su ranura de 3 × 70 mm y el canal de descarga impresos se
//  sustituyen por un TUBO DE BOTELLA con una ranura cortada a mano. El líquido no se
//  queda en ningún sitio: entra por arriba, sale por la ranura, cae por la camisa y
//  sale por la boquilla.
//
//  Piezas: cuerpo, caja_tapa, deflector y plantilla (guía para marcar la ranura).
//  Para exportar: elige "pieza", pulsa F6 y luego "Exportar como STL". PETG.
//  Las piezas comunes están en prototipo_banco.scad (misma carpeta).
// ============================================================================

pieza = "todo"; // [todo, cuerpo, caja_tapa, deflector, plantilla]

/* [Tubo de botella] */
botella_d   = 65;    // diámetro exterior de la botella: MÍDELO. Debe ser cilíndrica y recta
tubo_largo  = 98;    // largo del tubo cortado (arriba y abajo)
asiento     = 2.5;   // lo que entra el tubo en el suelo
n_ranuras   = 1;
ranura_b    = 3;     // ancho de la ranura
ranura_h    = 70;    // alto de la ranura, contado desde el suelo

/* [Camisa y salida] */
hueco        = 6;    // separación entre el suelo y la camisa
pared        = 2.4;
base_h       = 8;    // espesor del suelo
camisa_alto  = 82;   // sobre el suelo: más que la ranura
salida_d     = 12;
salida_ext   = 16;   // espigón para tubo de 12 mm interior
salida_largo = 8;
toma_x       = 12;   // toma junto a la ranura, como en la v0.1

/* [Caja de electrónica] (la de cad/uroflujometro.scad) */
caja_ancho = 46;
caja_fondo = 22;
pared_caja = 2.4;
caja_z0    = -10;
caja_z1    = 75;
oled_ancho = 24;
oled_alto  = 13;
oled_dz    = 30;     // la ventana empieza a esta distancia bajo la tapa
boton_dz   = 44;
usb        = "lateral"; // [suelo, lateral]
muesca     = true;
tapon_h    = 6;

include <prototipo_banco.scad>
