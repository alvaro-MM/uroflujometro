#pragma once
// ============================================================================
//  Configuración del uroflujómetro portátil v0.2 (solo volumen)
//  Placa: Seeed Studio XIAO ESP32-C3
// ============================================================================

// ---- Pines ------------------------------------------------------------------
#define PIN_BOTON        D1   // GPIO3 -> pulsador a GND (+ pull-up 10 k a 3V3)
#define PIN_BATERIA      A0   // GPIO2 -> divisor 2 x 220 k desde BAT+
#define PIN_SENSOR_VCC   D7   // GPIO20 -> alimenta sensor de presión y acelerómetro
// I2C por defecto de la XIAO: SDA = D4 (GPIO6), SCL = D5 (GPIO7)

#define OLED_DIR         0x3C
#define OLED_ALTO        32     // 32 para la OLED de 0,91" (128x32) o 64 para la de 0,96"
#define MS4525_DIR       0x28
#define LIS3DH_DIR       0x18   // el firmware prueba también 0x19
#define PRESION_SIGNO    (+1)   // pon -1 si el nivel sale negativo al llenar

// ---- Orientación del acelerómetro --------------------------------------------
// Ejes del aparato: Z = eje de la cámara hacia arriba, X = hacia la ranura 1,
// Y = hacia la ranura 2 (90° en sentido antihorario visto desde arriba).
// Cada fila dice cómo se obtiene un eje del aparato a partir de los ejes del LIS3DH.
// Ejemplo por defecto: placa de pie en la caja, con su eje Y hacia arriba,
// su eje X hacia la ranura 1 y su eje Z hacia fuera (hacia la ranura 4).
#define MAPA_ACC { { 1, 0,  0 },  \
                   { 0, 0, -1 },  \
                   { 0, 1,  0 } }

// ---- Geometría (debe coincidir con cad/uroflujometro_v02.scad) ---------------
#define N_RANURAS         4
#define RANURA_ANCHO_MM   1.5f
#define RANURA_ALTO_MM    36.0f
#define RADIO_RANURAS_MM  12.0f   // radio interior de la cámara
#define REBOSE_MM         40.0f   // ventanas de rebose interno (a 45° de las ranuras)
#define CD_RANURA         0.62f   // coeficiente de descarga teórico (se calibra)

// ---- Algoritmo (alturas en mm de columna de agua) ----------------------------
#define FS_HZ             40
#define MEDIA_MUESTRAS    8       // media móvil (0,2 s) de presión y aceleración
#define H_INICIO_MM       1.5f    // nivel que dispara la medida...
#define SUBIDA_INICIO_MM  0.8f    // ...si además ha subido esto en 1 s
#define H_FIN_MM          0.8f    // por debajo, la cámara se considera vacía
#define T_FIN_S           4.0f    // segundos vacía para cerrar la medida
#define H_SECO_MM         0.4f    // toma seca: no se sabe dónde está el agua -> caudal 0
#define H_MIN_Q_MM        0.3f    // carga mínima por ranura para contar caudal
#define V_MIN_GUARDAR     20.0f   // mL; por debajo se descarta (gotas, enjuague)
#define T_MAX_MEDIDA_S    300.0f
#define INCL_MAX_GRADOS   25.0f   // por encima: aviso "INCLINADO" y medida dudosa

#define DENSIDAD_ORINA    1.015f

// ---- Interfaz ----------------------------------------------------------------
#define T_RESULTADO_S        180   // el resultado se queda en pantalla para apuntarlo
#define T_APAGADO_S          300
#define T_PULSACION_LARGA_MS 1500
#define V_BATERIA_MIN        3.30f
#define HISTORIAL_N          30
