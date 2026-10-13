#pragma once
// ============================================================================
//  Configuración del uroflujómetro de vertedero (prototipo v0.1)
//  Placa: Seeed Studio XIAO ESP32-C3
// ============================================================================

// ---- Sensor de presión: deja UNO sin comentar ------------------------------
#define SENSOR_MS4525DO     // I2C, ±1 psi diferencial, 14 bits. Recomendado.
// #define SENSOR_HX710B    // Módulo barato 0-40 kPa con HX710B. Más ruido.

// ---- Pines ------------------------------------------------------------------
#define PIN_BOTON        D1   // GPIO3 -> pulsador a GND (+ pull-up 10 k a 3V3)
#define PIN_BATERIA      A0   // GPIO2 -> divisor 2 x 220 k desde BAT+
#define PIN_SENSOR_VCC   D7   // GPIO20 -> alimenta el sensor (se apaga al dormir)
#define PIN_HX_SCK       D2   // GPIO4 (solo HX710B)
#define PIN_HX_OUT       D3   // GPIO5 (solo HX710B)
// I2C por defecto de la XIAO: SDA = D4 (GPIO6), SCL = D5 (GPIO7)

#define OLED_DIR         0x3C
#define MS4525_DIR       0x28
#define PRESION_SIGNO    (+1)  // pon -1 si el nivel sale negativo al llenar

// Escala inicial del sensor (mm de columna de agua por unidad leída).
// MS4525DO ya viene calibrado de fábrica: 1.0. HX710B: ~1/5000 cuentas; se
// ajusta con el comando serie "escala <mm>".
#if defined(SENSOR_HX710B)
  #define ESCALA_DEFECTO (1.0f / 5000.0f)
#else
  #define ESCALA_DEFECTO 1.0f
#endif

// ---- Geometría de la cámara (debe coincidir con cad/uroflujometro.scad) ----
#define RANURA_ANCHO_MM   3.0f
#define RANURA_ALTO_MM    70.0f
#define REBOSE_MM         78.0f   // ventanas de rebose interno
#define CD_RANURA         0.62f   // coeficiente de descarga teórico (se calibra)

// ---- Algoritmo (alturas en mm de columna) ----------------------------------
#define FS_HZ             40      // muestreo del sensor
#define MEDIA_MUESTRAS    8       // media móvil del nivel (0,2 s)
#define H_INICIO_MM       2.0f    // nivel que dispara la medida...
#define SUBIDA_INICIO_MM  1.0f    // ...si además ha subido esto en 1 s
#define H_FIN_MM          1.0f    // por debajo, la cámara se considera vacía
#define T_FIN_S           5.0f    // segundos vacía para cerrar la medida
#define H_MIN_Q_MM        0.3f    // por debajo, caudal 0 (no integrar ruido)
#define Q_FLUJO_MIN       0.5f    // mL/s mínimos para contar tiempo de flujo
#define V_MIN_GUARDAR     20.0f   // mL; por debajo se descarta (gotas, enjuague)
#define T_MAX_MEDIDA_S    300.0f

#define DENSIDAD_ORINA    1.015f  // densidad relativa típica (1,005 - 1,030)

// ---- Interfaz ----------------------------------------------------------------
#define T_RESULTADO_S        120   // tiempo mostrando el resultado
#define T_APAGADO_S          300   // apagado por inactividad
#define T_PULSACION_LARGA_MS 1500  // pulsación larga = apagar
#define V_BATERIA_MIN        3.30f // por debajo se apaga
#define HISTORIAL_N          20
