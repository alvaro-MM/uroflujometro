// ============================================================================
//  Prototipo de banco v0.2 · electrónica + tubo de botella · OpenSCAD
//
//  Para montar en casa la v0.2 antes de tener el chasis definitivo. Lleva la misma
//  caja de electrónica (XIAO ESP32-C3, MS4525DO, LIS3DH, OLED de 0,91", botón y batería),
//  pero la cámara de Ø24 y sus 4 ranuras impresas se sustituyen por un TUBO DE BOTELLA
//  con 4 ranuras cortadas a mano. El líquido no se queda en ningún sitio: entra por
//  arriba, sale por las ranuras, cae por la camisa y sale por la boquilla.
//
//  Piezas: cuerpo, caja_tapa, deflector y plantilla (guía para marcar las ranuras).
//  Para exportar: elige "pieza", pulsa F6 y luego "Exportar como STL". PETG.
//  Las piezas comunes están en prototipo_banco.scad (misma carpeta).
// ============================================================================

pieza = "todo"; // [todo, cuerpo, caja_tapa, deflector, plantilla]

/* [Tubo de botella] */
botella_d   = 65;    // diámetro exterior de la botella: MÍDELO. Debe ser cilíndrica y recta
tubo_largo  = 51;    // largo del tubo cortado (arriba y abajo)
asiento     = 2.5;   // lo que entra el tubo en el suelo
n_ranuras   = 4;
ranura_b    = 1.5;   // ancho de cada ranura
ranura_h    = 36;    // alto de cada ranura, contado desde el suelo

/* [Camisa y salida] */
hueco        = 6;    // separación entre el suelo y la camisa
pared        = 2;
base_h       = 8;    // espesor del suelo
camisa_alto  = 48;   // sobre el suelo: más que las ranuras
salida_d     = 10;
salida_ext   = 13;
salida_largo = 8;
toma_x       = 0;    // la toma va en el centro del suelo, como en la v0.2

/* [Caja de electrónica] (la de cad/uroflujometro_v02.scad) */
caja_ancho = 40;
caja_fondo = 14;
pared_caja = 2;
caja_z0    = -10;
caja_z1    = 48;
oled_ancho = 24;
oled_alto  = 8;
oled_dz    = 12;     // la ventana empieza a esta distancia bajo la tapa
boton_dz   = 24;
usb        = "suelo"; // [suelo, lateral]
muesca     = false;
tapon_h    = 5;

include <prototipo_banco.scad>
