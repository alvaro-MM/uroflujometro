#pragma once
// Tipos compartidos. Van en un .h para que los prototipos que genera el IDE de
// Arduino los encuentren ya declarados.
#include <stdint.h>

struct Ajustes {
  float a;         // coeficiente de Q = a·h^n (h en mm, Q en mL/s)
  float n;         // exponente
  float densidad;  // densidad relativa de la orina
  float escala;    // mm de columna de agua por unidad del sensor
};

enum Estado { LISTO, MIDIENDO, RESULTADO };

struct Medida {
  float vol;       // mL
  float qmax;      // mL/s (media de 2 s)
  float tQmax;     // s desde el inicio
  float qmed;      // mL/s = vol / tiempo de flujo
  float tFlujo;    // s con caudal > Q_FLUJO_MIN
  float tMiccion;  // s desde el inicio hasta el último flujo
  bool aviso;      // el nivel se acercó al tope de la ranura
  bool rebose;     // pasó por el rebose interno: medida no válida
};

struct Registro { uint32_t num; float vol, qmax, qmed, tMiccion; uint8_t flags; };

const int NGRID = 9;                 // exponentes probados al calibrar
struct Vertido { float v; float s[NGRID]; };
