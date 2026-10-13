// ============================================================================
//  Uroflujómetro portátil v0.2 · solo volumen
//
//  La orina cae por un buzo con tapón a una cámara de Ø24 mm que se vacía por
//  4 ranuras verticales repartidas alrededor. Un sensor de presión mide la
//  profundidad en el centro del suelo y un acelerómetro mide la inclinación y la
//  gravedad aparente. Con eso se calcula la carga de cada ranura:
//      d_i = d_centro − r·(arriba · dirección_i)
//      Q   = Σ a/N · √(g) · d_i^n / cos α_i
//  Volumen = ∫ Q dt. Funciona sujeto con la mano hasta unos 25° de inclinación.
//
//  Placa: Seeed XIAO ESP32-C3 (Arduino-ESP32 2.x o 3.x, "USB CDC On Boot" activado)
//  Librerías: Adafruit SSD1306 + Adafruit GFX
//  Comandos por el monitor serie (115200 baudios): escribe "ayuda".
// ============================================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <esp_sleep.h>
#include "config.h"
#include "tipos.h"

Adafruit_SSD1306 oled(128, OLED_ALTO, &Wire, -1);
Preferences prefs;

// ---------------------------------------------------------------- ajustes ----
Ajustes aj;

float coefTeorico() {
  // 2/3 · Cd · (N·b) · √(2g), en SI, pasado a mL/s con h en mm
  return (2.0f / 3.0f) * CD_RANURA * (N_RANURAS * RANURA_ANCHO_MM / 1000.0f) * sqrtf(2.0f * 9.81f) * 1e6f / powf(1000.0f, 1.5f);
}

void ajustesFabrica() {
  aj.a = coefTeorico();
  aj.n = 1.5f;
  aj.densidad = DENSIDAD_ORINA;
  aj.nivel[0] = aj.nivel[1] = 0;
}

void cargarAjustes() {
  ajustesFabrica();
  aj.a = prefs.getFloat("a", aj.a);
  aj.n = prefs.getFloat("n", aj.n);
  aj.densidad = prefs.getFloat("dens", aj.densidad);
  aj.nivel[0] = prefs.getFloat("nx", 0);
  aj.nivel[1] = prefs.getFloat("ny", 0);
}

void guardarAjustes() {
  prefs.putFloat("a", aj.a);
  prefs.putFloat("n", aj.n);
  prefs.putFloat("dens", aj.densidad);
  prefs.putFloat("nx", aj.nivel[0]);
  prefs.putFloat("ny", aj.nivel[1]);
}

// -------------------------------------------------------------- sensores ----
// Presión en mm de columna de agua (sin restar el cero). MS4525DO, ±1 psi, salida tipo A.
bool leerPresion(float &hw) {
  if (Wire.requestFrom((uint8_t)MS4525_DIR, (uint8_t)4) != 4) return false;
  uint8_t b0 = Wire.read(), b1 = Wire.read();
  Wire.read(); Wire.read();
  if ((b0 >> 6) == 3) return false;
  uint16_t raw = ((uint16_t)(b0 & 0x3F) << 8) | b1;
  float psi = ((float)raw - 1638.3f) * 2.0f / 13106.4f - 1.0f;
  hw = PRESION_SIGNO * psi * 6894.757f / 9.80665f;
  return true;
}

uint8_t dirAcc = 0;
const int8_t MAPA[3][3] = MAPA_ACC;

void escribirAcc(uint8_t reg, uint8_t v) {
  Wire.beginTransmission(dirAcc); Wire.write(reg); Wire.write(v); Wire.endTransmission();
}

bool iniciarAcc() {
  const uint8_t dirs[2] = { LIS3DH_DIR, (uint8_t)(LIS3DH_DIR ^ 0x01) };
  for (uint8_t d : dirs) {
    Wire.beginTransmission(d); Wire.write(0x0F);                  // WHO_AM_I
    if (Wire.endTransmission(false) != 0) continue;
    if (Wire.requestFrom(d, (uint8_t)1) == 1 && Wire.read() == 0x33) {
      dirAcc = d;
      escribirAcc(0x20, 0x57);   // CTRL_REG1: 100 Hz, ejes X, Y, Z
      escribirAcc(0x23, 0x88);   // CTRL_REG4: BDU, ±2 g, alta resolución (1 mg)
      return true;
    }
  }
  return false;
}

// Aceleración en g, ya en ejes del aparato
bool leerAcc(float a[3]) {
  if (!dirAcc) return false;
  Wire.beginTransmission(dirAcc); Wire.write(0x28 | 0x80);       // OUT_X_L con autoincremento
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(dirAcc, (uint8_t)6) != 6) return false;
  float s[3];
  for (int i = 0; i < 3; i++) {
    uint8_t lo = Wire.read();                                      // el orden importa: primero el byte bajo
    uint8_t hi = Wire.read();
    int16_t v = (int16_t)(lo | (hi << 8));
    s[i] = (v >> 4) / 1000.0f;                                     // 12 bits, 1 mg por cuenta
  }
  for (int i = 0; i < 3; i++) a[i] = MAPA[i][0] * s[0] + MAPA[i][1] * s[1] + MAPA[i][2] * s[2];
  return true;
}

// ----------------------------------------------------------------- estado ----
Estado estado = LISTO;
Medida med, ultima;
bool hayUltima = false, accOk = false;

float cero = 0;
float bufP[MEDIA_MUESTRAS], bufA[MEDIA_MUESTRAS][3];
int iMedia = 0, nMedia = 0;
float hFilt = 0;                // presión filtrada (mm de agua a 1 g)
Postura pos = { { 0, 0, 1 }, 1, 0 };
float qAct = 0;
float hist1s[FS_HZ];
int iHist = 0;
float tMedida = 0, tCalma = 0, tUltFlujo = 0;

const float N_GRID[NGRID] = { 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 1.9f };
float sumaN[NGRID];
const int MAX_VERTIDOS = 10;
Vertido vertidos[MAX_VERTIDOS];
int nVertidos = 0;

uint32_t tResultadoMs = 0, tActividadMs = 0, tPantallaMs = 0, tBateriaMs = 0, tTrazaMs = 0;
float vBateria = 4.0f;
bool traza = false;

// ----------------------------------------------------------------- modelo ----
// Profundidad bajo la superficie de cada punto del borde a un ángulo f
float cargaEn(float dc, float f, const Postura &p) {
  return dc - RADIO_RANURAS_MM * (p.up[0] * cosf(f) + p.up[1] * sinf(f));
}

// Suma de las ranuras para un exponente n y coeficiente a. "dc" en mm (densidad ya aplicada).
float caudalRanuras(float dc, const Postura &p, float a, float n) {
  float q = 0;
  for (int i = 0; i < N_RANURAS; i++) {
    float f = i * 2.0f * PI / N_RANURAS;
    float di = cargaEn(dc, f, p);
    if (di <= H_MIN_Q_MM) continue;
    float tanA = (-p.up[0] * sinf(f) + p.up[1] * cosf(f)) / p.up[2];
    q += a / N_RANURAS * sqrtf(p.g) * powf(di, n) * sqrtf(1.0f + tanA * tanA);
  }
  return q;
}

void calcularPostura(const float a[3]) {
  float g = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
  if (g < 0.3f) return;                           // caída libre o lectura absurda
  float ux = a[0] / g - aj.nivel[0], uy = a[1] / g - aj.nivel[1], uz = a[2] / g;
  float m = sqrtf(ux * ux + uy * uy + uz * uz);
  pos.up[0] = ux / m; pos.up[1] = uy / m; pos.up[2] = uz / m;
  pos.g = g;
  pos.incl = acosf(constrain(pos.up[2], -1.0f, 1.0f)) * 180.0f / PI;
}

// ----------------------------------------------------------------- medida ----
void reiniciarFiltro() {
  nMedia = 0; iMedia = 0;
  for (int i = 0; i < FS_HZ; i++) hist1s[i] = 0;
}

void ajustarCero(uint16_t ms) {
  float s = 0; int n = 0;
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    float hw;
    if (leerPresion(hw)) { s += hw; n++; }
    delay(5);
  }
  if (n) cero = s / n;
  reiniciarFiltro();
}

void empezarMedida() {
  estado = MIDIENDO;
  memset(&med, 0, sizeof(med));
  tMedida = 0; tCalma = 0; tUltFlujo = 0;
  for (int k = 0; k < NGRID; k++) sumaN[k] = 0;
  Serial.println(F("# inicio de medida"));
  Serial.println(F("t_s,h_mm,incl_grados,q_mLs,v_mL"));
}

void terminarMedida() {
  med.tMiccion = tUltFlujo;
  med.inclinado = med.inclMax > INCL_MAX_GRADOS;
  tActividadMs = millis();
  if (med.vol < V_MIN_GUARDAR) {
    Serial.printf("# descartada: %.1f mL (< %.0f mL)\n", med.vol, V_MIN_GUARDAR);
    estado = LISTO;
    return;
  }
  ultima = med;
  hayUltima = true;
  guardarHistorial(ultima);
  Serial.printf("# resultado: V=%.0f mL  t=%.1f s  inclinacion max=%.0f grados%s%s\n",
                ultima.vol, ultima.tMiccion, ultima.inclMax,
                ultima.inclinado ? "  [INCLINADO: dudosa]" : "", ultima.rebose ? "  [REBOSE: no valida]" : "");
  estado = RESULTADO;
  tResultadoMs = millis();
}

void acumularMedida(float hBruto, float dt) {
  tMedida += dt;
  med.vol += qAct * dt;
  if (pos.incl > med.inclMax) med.inclMax = pos.incl;

  // Sumas para calibrar con agua (densidad 1)
  float dcAgua = max(0.0f, hFilt) / pos.g;
  if (dcAgua > H_SECO_MM)
    for (int k = 0; k < NGRID; k++) sumaN[k] += caudalRanuras(dcAgua, pos, 1.0f, N_GRID[k]) * dt;

  if (qAct > 0.5f) tUltFlujo = tMedida;
  // Rebose y aviso con la geometría: ventanas a 45° de las ranuras
  float dc = max(0.0f, hBruto) / (aj.densidad * pos.g);
  for (int i = 0; i < N_RANURAS; i++) {
    float f = i * 2.0f * PI / N_RANURAS;
    if (cargaEn(dc, f + PI / N_RANURAS, pos) > REBOSE_MM * pos.up[2]) med.rebose = true;
    if (cargaEn(dc, f, pos) > (RANURA_ALTO_MM - 4) * pos.up[2]) med.aviso = true;
  }

  static float tTraza = 0;
  tTraza += dt;
  if (tTraza >= 0.1f) {
    tTraza = 0;
    Serial.printf("%.1f,%.2f,%.1f,%.2f,%.1f\n", tMedida, hFilt, pos.incl, qAct, med.vol);
  }

  tCalma = hFilt < H_FIN_MM ? tCalma + dt : 0;
  if (tCalma >= T_FIN_S || tMedida > T_MAX_MEDIDA_S) terminarMedida();
}

void procesarMuestra(float hw, const float acc[3], bool hayAcc, float dt) {
  bufP[iMedia] = hw - cero;
  if (hayAcc) for (int k = 0; k < 3; k++) bufA[iMedia][k] = acc[k];
  else { bufA[iMedia][0] = 0; bufA[iMedia][1] = 0; bufA[iMedia][2] = 1; }
  iMedia = (iMedia + 1) % MEDIA_MUESTRAS;
  if (nMedia < MEDIA_MUESTRAS) nMedia++;
  float s = 0, a[3] = { 0, 0, 0 };
  for (int i = 0; i < nMedia; i++) { s += bufP[i]; for (int k = 0; k < 3; k++) a[k] += bufA[i][k]; }
  hFilt = s / nMedia;
  for (int k = 0; k < 3; k++) a[k] /= nMedia;
  calcularPostura(a);

  float subida = hFilt - hist1s[iHist];
  hist1s[iHist] = hFilt;
  iHist = (iHist + 1) % FS_HZ;

  // Profundidad en el centro con la densidad de la orina y la gravedad aparente
  float dc = max(0.0f, hFilt) / (aj.densidad * pos.g);
  qAct = dc > H_SECO_MM ? caudalRanuras(dc, pos, aj.a, aj.n) : 0;

  switch (estado) {
    case LISTO:
      if (fabsf(subida) < 0.3f && hFilt < 4.0f) cero += 0.01f * (hw - cero);
      if (hFilt > H_INICIO_MM && subida > SUBIDA_INICIO_MM) empezarMedida();
      break;
    case MIDIENDO:
      acumularMedida(hw - cero, dt);
      break;
    case RESULTADO:
      if (millis() - tResultadoMs > T_RESULTADO_S * 1000UL && hFilt < H_FIN_MM) {
        estado = LISTO;
        tActividadMs = millis();
      }
      break;
  }
}

// ------------------------------------------------------------- historial ----
int leerHistorial(Registro *h) {
  size_t len = prefs.getBytesLength("hist");
  if (len == 0 || len > sizeof(Registro) * HISTORIAL_N) return 0;
  prefs.getBytes("hist", h, len);
  return len / sizeof(Registro);
}

void guardarHistorial(const Medida &m) {
  Registro h[HISTORIAL_N];
  int cuenta = leerHistorial(h);
  uint32_t num = prefs.getUInt("num", 0) + 1;
  prefs.putUInt("num", num);
  if (cuenta == HISTORIAL_N) { memmove(h, h + 1, sizeof(Registro) * (HISTORIAL_N - 1)); cuenta--; }
  h[cuenta++] = { num, m.vol, m.tMiccion, m.inclMax,
                  (uint8_t)((m.rebose ? 1 : 0) | (m.inclinado ? 2 : 0) | (m.aviso ? 4 : 0)) };
  prefs.putBytes("hist", h, sizeof(Registro) * cuenta);
}

void imprimirHistorial() {
  Registro h[HISTORIAL_N];
  int cuenta = leerHistorial(h);
  Serial.println(F("num,vol_mL,t_s,incl_max,notas"));
  for (int i = 0; i < cuenta; i++)
    Serial.printf("%lu,%.0f,%.1f,%.0f,%s%s\n", (unsigned long)h[i].num, h[i].vol, h[i].tMiccion, h[i].inclMax,
                  (h[i].flags & 1) ? "rebose " : "", (h[i].flags & 2) ? "inclinado" : "");
}

// ------------------------------------------------------------- pantalla ----
const bool GRANDE = OLED_ALTO >= 64;

void pintarBateria() {
  float f = constrain((vBateria - 3.3f) / 0.9f, 0.0f, 1.0f);
  oled.drawRect(112, 0, 13, 7, SSD1306_WHITE);
  oled.fillRect(125, 2, 2, 3, SSD1306_WHITE);
  oled.fillRect(114, 2, (int)(f * 9 + 0.5f), 3, SSD1306_WHITE);
}

void numeroGrande(float v, int y) {
  oled.setTextSize(GRANDE ? 4 : 3);
  oled.setCursor(0, y);
  oled.print((int)roundf(v));
  oled.setTextSize(1);
  oled.print(F(" mL"));
}

void pantallaListo() {
  pintarBateria();
  oled.setTextSize(2); oled.setCursor(0, GRANDE ? 14 : 0); oled.print(F("LISTO"));
  oled.setTextSize(1); oled.setCursor(0, GRANDE ? 40 : 22);
  if (!accOk) oled.print(F("SIN ACELEROMETRO"));
  else if (pos.incl > INCL_MAX_GRADOS) oled.print(F("ENDEREZALO"));
  else oled.printf("Orina  incl %.0f", pos.incl);
}

void pantallaMidiendo() {
  pintarBateria();
  numeroGrande(med.vol, GRANDE ? 14 : 4);
  oled.setTextSize(1);
  oled.setCursor(GRANDE ? 0 : 84, GRANDE ? 54 : 12);
  bool parpadeo = (millis() / 300) % 2;
  if (pos.incl > INCL_MAX_GRADOS) { if (parpadeo) oled.print(F("INCLINADO")); }
  else if (med.aviso) { if (parpadeo) oled.print(F("LLENO!")); }
  else oled.printf("%.0f grad", pos.incl);
}

void pantallaResultado() {
  pintarBateria();
  numeroGrande(ultima.vol, GRANDE ? 14 : 4);
  oled.setTextSize(1);
  oled.setCursor(GRANDE ? 0 : 84, GRANDE ? 54 : 20);
  if (ultima.rebose) oled.print(F("REBOSE"));
  else if (ultima.inclinado) oled.print(F("DUDOSA"));
  else oled.print(F("apuntar"));
}

void pintarPantalla() {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  if (estado == LISTO) pantallaListo();
  else if (estado == MIDIENDO) pantallaMidiendo();
  else pantallaResultado();
  oled.display();
}

void mensaje(const char *l1, const char *l2) {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(2); oled.setCursor(0, 0); oled.print(l1);
  oled.setTextSize(1); oled.setCursor(0, GRANDE ? 40 : 22); oled.print(l2);
  oled.display();
}

// ------------------------------------------------------- energía y botón ----
float leerBateria() { return analogReadMilliVolts(PIN_BATERIA) * 2.0f / 1000.0f; }

void apagar() {
  mensaje("Apagando", "Pulsa para encender");
  while (digitalRead(PIN_BOTON) == LOW) delay(10);
  delay(800);
  oled.ssd1306_command(SSD1306_DISPLAYOFF);
  digitalWrite(PIN_SENSOR_VCC, LOW);
  esp_deep_sleep_enable_gpio_wakeup(1ULL << PIN_BOTON, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}

void pulsacionCorta() {
  tActividadMs = millis();
  if (estado == LISTO) {
    mensaje("Cero...", "Camara vacia");
    ajustarCero(1000);
  } else if (estado == RESULTADO) {
    estado = LISTO;
    reiniciarFiltro();
  }
}

void leerBoton() {
  static bool antes = HIGH;
  static uint32_t tCambio = 0, tPulsado = 0;
  static bool largoHecho = false;
  bool ahora = digitalRead(PIN_BOTON);
  if (ahora != antes && millis() - tCambio > 30) {
    tCambio = millis();
    if (ahora == LOW) { tPulsado = millis(); largoHecho = false; }
    else if (!largoHecho) pulsacionCorta();
    antes = ahora;
  }
  if (antes == LOW && !largoHecho && millis() - tPulsado > T_PULSACION_LARGA_MS) {
    largoHecho = true;
    apagar();
  }
}

// ------------------------------------------------------- comandos serie ----
int indiceN(float n) {
  int best = 0;
  for (int k = 1; k < NGRID; k++) if (fabsf(N_GRID[k] - n) < fabsf(N_GRID[best] - n)) best = k;
  return best;
}

void ajustarCurva() {
  if (nVertidos == 0) { Serial.println(F("Primero registra vertidos con: cal <mL>")); return; }
  int mejor = indiceN(aj.n);
  float aMejor = 0, cvMejor = 1e9;
  for (int k = 0; k < NGRID; k++) {
    float m = 0, m2 = 0;
    for (int i = 0; i < nVertidos; i++) { float ak = vertidos[i].v / vertidos[i].s[k]; m += ak; m2 += ak * ak; }
    m /= nVertidos;
    float cv = sqrtf(max(0.0f, m2 / nVertidos - m * m)) / m;
    Serial.printf("  n=%.1f  a=%.5f  dispersion=%.2f %%\n", N_GRID[k], m, cv * 100);
    if (nVertidos == 1 ? (k == mejor) : (cv < cvMejor)) { cvMejor = cv; aMejor = m; mejor = k; }
  }
  aj.n = N_GRID[mejor];
  aj.a = aMejor;
  guardarAjustes();
  Serial.printf("Guardado: Q = %.5f * suma(h^%.1f)\n", aj.a, aj.n);
  for (int i = 0; i < nVertidos; i++) {
    float v = aj.a * vertidos[i].s[mejor];
    Serial.printf("  vertido %d: real %.0f mL, modelo %.1f mL (%+.1f mL)\n", i + 1, vertidos[i].v, v, v - vertidos[i].v);
  }
  if (nVertidos < 4) Serial.println(F("Consejo: haz al menos 4 vertidos a ritmos distintos, alguno con el aparato inclinado."));
}

void imprimirEstado() {
  Serial.printf("Acelerometro: %s\n", accOk ? "si" : "NO DETECTADO");
  Serial.printf("Q = %.5f * suma(h^%.2f)  (teorico: %.5f * h^1.5)\n", aj.a, aj.n, coefTeorico());
  Serial.printf("densidad %.3f  cero %.3f  nivel (%.4f, %.4f)\n", aj.densidad, cero, aj.nivel[0], aj.nivel[1]);
  Serial.printf("presion %.2f mm  inclinacion %.1f grados  g %.3f\n", hFilt, pos.incl, pos.g);
  Serial.printf("arriba = (%.3f, %.3f, %.3f)  bateria %.2f V\n", pos.up[0], pos.up[1], pos.up[2], vBateria);
  Serial.printf("vertidos registrados: %d\n", nVertidos);
}

void nivelar() {
  // Con el aparato recto sobre una mesa: lo que quede de inclinación es error de montaje
  float sx = 0, sy = 0; int n = 0;
  uint32_t t0 = millis();
  while (millis() - t0 < 1500) {
    float a[3];
    if (leerAcc(a)) {
      float g = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
      if (g > 0.5f) { sx += a[0] / g; sy += a[1] / g; n++; }
    }
    delay(10);
  }
  if (!n) { Serial.println(F("No se puede leer el acelerometro")); return; }
  float nx = sx / n, ny = sy / n;
  float grados = asinf(constrain(sqrtf(nx * nx + ny * ny), 0.0f, 1.0f)) * 180 / PI;
  if (grados > 10) { Serial.printf("Desviacion de %.1f grados: revisa MAPA_ACC en config.h\n", grados); return; }
  aj.nivel[0] = nx; aj.nivel[1] = ny;
  guardarAjustes();
  Serial.printf("Nivel guardado (desviacion de montaje %.2f grados)\n", grados);
}

void comando(String c) {
  c.trim();
  if (!c.length()) return;
  int esp = c.indexOf(' ');
  String cmd = esp < 0 ? c : c.substring(0, esp);
  float arg = esp < 0 ? NAN : c.substring(esp + 1).toFloat();
  cmd.toLowerCase();

  if (cmd == "ayuda") {
    Serial.println(F("cero            ajusta el cero de presion (camara vacia)"));
    Serial.println(F("nivel           guarda la inclinacion de montaje (aparato recto sobre una mesa)"));
    Serial.println(F("estado          parametros y lectura actual"));
    Serial.println(F("traza           salida continua de presion, inclinacion y caudal"));
    Serial.println(F("cal <mL>        registra la ultima medida como vertido de agua de <mL> conocidos"));
    Serial.println(F("ajustar         calcula a y n con los vertidos registrados"));
    Serial.println(F("borrarcal       borra los vertidos registrados"));
    Serial.println(F("densidad <d>    densidad relativa de la orina (1.000-1.040)"));
    Serial.println(F("historial       ultimas medidas"));
    Serial.println(F("fabrica         vuelve a los valores teoricos"));
  } else if (cmd == "cero") {
    ajustarCero(1000);
    Serial.printf("Cero = %.3f\n", cero);
  } else if (cmd == "nivel") {
    nivelar();
  } else if (cmd == "estado") {
    imprimirEstado();
  } else if (cmd == "traza") {
    traza = !traza;
    Serial.println(traza ? F("traza activada (h_mm, incl, q_mLs)") : F("traza desactivada"));
  } else if (cmd == "cal") {
    if (isnan(arg) || arg <= 0) { Serial.println(F("Uso: cal <mL>  (gramos de agua vertidos)")); return; }
    if (!hayUltima) { Serial.println(F("No hay ninguna medida terminada")); return; }
    if (nVertidos >= MAX_VERTIDOS) { Serial.println(F("Lista llena: usa borrarcal")); return; }
    vertidos[nVertidos].v = arg;
    memcpy(vertidos[nVertidos].s, sumaN, sizeof(sumaN));
    nVertidos++;
    float comoAgua = aj.a * sumaN[indiceN(aj.n)];
    Serial.printf("Vertido %d: real %.0f mL, medido como agua %.1f mL (%+.1f mL), inclinacion max %.0f\n",
                  nVertidos, arg, comoAgua, comoAgua - arg, ultima.inclMax);
  } else if (cmd == "ajustar") {
    ajustarCurva();
  } else if (cmd == "borrarcal") {
    nVertidos = 0;
    Serial.println(F("Vertidos borrados"));
  } else if (cmd == "densidad") {
    if (isnan(arg) || arg < 1.0f || arg > 1.04f) { Serial.println(F("Uso: densidad 1.015")); return; }
    aj.densidad = arg;
    guardarAjustes();
    Serial.printf("Densidad = %.3f\n", aj.densidad);
  } else if (cmd == "historial") {
    imprimirHistorial();
  } else if (cmd == "fabrica") {
    ajustesFabrica();
    guardarAjustes();
    Serial.println(F("Ajustes teoricos restaurados"));
  } else {
    Serial.println(F("Comando desconocido. Escribe: ayuda"));
  }
}

void leerSerie() {
  static String linea;
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') { comando(linea); linea = ""; }
    else if (linea.length() < 40) linea += ch;
  }
}

// ----------------------------------------------------------- setup/loop ----
void setup() {
  Serial.begin(115200);
  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_SENSOR_VCC, OUTPUT);
  digitalWrite(PIN_SENSOR_VCC, HIGH);
  analogReadResolution(12);
  Wire.begin();
  Wire.setClock(400000);
  prefs.begin("uroflujo2", false);
  cargarAjustes();

  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_DIR)) Serial.println(F("! No se encuentra la pantalla OLED"));
  mensaje("Uroflujo", "Camara vacia: cero");
  while (digitalRead(PIN_BOTON) == LOW) delay(10);
  delay(300);

  accOk = iniciarAcc();
  if (!accOk) Serial.println(F("! Acelerometro no detectado: se mide como si estuviera recto"));
  float prueba;
  if (!leerPresion(prueba)) { delay(100); if (!leerPresion(prueba)) Serial.println(F("! El sensor de presion no responde")); }
  ajustarCero(1000);
  vBateria = leerBateria();
  tActividadMs = millis();
  Serial.println(F("Uroflujometro v0.2 listo. Escribe 'ayuda' para ver los comandos."));
}

void loop() {
  uint32_t ahora = millis();
  static uint32_t tPrev = 0;

  if (ahora - tPrev >= 1000 / FS_HZ) {
    float hw, acc[3];
    if (leerPresion(hw)) {
      bool hayAcc = leerAcc(acc);
      float dt = tPrev ? (ahora - tPrev) / 1000.0f : 1.0f / FS_HZ;
      if (dt > 0.2f) dt = 0.2f;
      tPrev = ahora;
      procesarMuestra(hw, acc, hayAcc, dt);
    }
  }

  leerBoton();
  leerSerie();

  if (traza && estado != MIDIENDO && ahora - tTrazaMs >= 100) {
    tTrazaMs = ahora;
    Serial.printf("%.2f,%.1f,%.2f\n", hFilt, pos.incl, qAct);
  }
  if (ahora - tPantallaMs >= 200) { tPantallaMs = ahora; pintarPantalla(); }
  if (ahora - tBateriaMs >= 5000) {
    tBateriaMs = ahora;
    vBateria = leerBateria();
    if (estado == LISTO && vBateria > 2.5f && vBateria < V_BATERIA_MIN) {
      mensaje("Bateria", "Carga por USB-C");
      delay(2000);
      apagar();
    }
  }
  if (estado == LISTO && ahora - tActividadMs > T_APAGADO_S * 1000UL) apagar();
}
