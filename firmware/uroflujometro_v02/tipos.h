#pragma once
// Tipos compartidos. Van en un .h para que los prototipos que genera el IDE de
// Arduino los encuentren ya declarados.
#include <stdint.h>

struct Ajustes {
  float a;          // coeficiente de Q = a·h^n para todas las ranuras juntas
  float n;          // exponente
  float densidad;   // densidad relativa de la orina
  float nivel[2];   // inclinación residual del acelerómetro con el aparato recto (x, y)
};

enum Estado { LISTO, MIDIENDO, RESULTADO };

struct Medida {
  float vol;        // mL
  float tMiccion;   // s desde el inicio hasta el último flujo
  float inclMax;    // grados
  bool aviso;       // el nivel se acercó al tope de las ranuras
  bool rebose;      // pasó por el rebose interno: medida no válida
  bool inclinado;   // pasó de INCL_MAX_GRADOS: medida dudosa
};

struct Registro { uint32_t num; float vol, tMiccion, inclMax; uint8_t flags; };

const int NGRID = 9;                 // exponentes probados al calibrar
struct Vertido { float v; float s[NGRID]; };

struct Postura { float up[3]; float g; float incl; };   // vector "arriba", gravedad aparente (g), grados
