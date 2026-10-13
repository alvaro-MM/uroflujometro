// ============================================================================
//  Uroflujómetro de vertedero · prototipo v0.1
//
//  Principio: la orina cae en una cámara con una ranura vertical. El caudal que
//  sale por la ranura depende solo del nivel h:  Q = a · h^n  (h en mm, Q en mL/s).
//  Un sensor de presión mide h a través de una columna de aire (no toca la orina).
//  Volumen = ∫ Q dt.  Qmax = máximo de la media móvil de 2 s (regla ICS).
//
//  Placa: Seeed XIAO ESP32-C3 (Arduino-ESP32 2.x o 3.x, "USB CDC On Boot" activado)
//  Librerías: Adafruit SSD1306 + Adafruit GFX
//
//  Comandos por el monitor serie (115200 baudios, fin de línea \n): escribe "ayuda".
// ============================================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <esp_sleep.h>
#include "config.h"
#include "tipos.h"

#if !defined(SENSOR_MS4525DO) && !defined(SENSOR_HX710B)
  #error "Elige un sensor en config.h"
#endif

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
Preferences prefs;

// ---------------------------------------------------------------- ajustes ----
Ajustes aj;

float coefTeorico() {
  // 2/3 · Cd · b · √(2g), en SI, pasado a mL/s con h en mm
  return (2.0f / 3.0f) * CD_RANURA * (RANURA_ANCHO_MM / 1000.0f) * sqrtf(2.0f * 9.81f) * 1e6f / powf(1000.0f, 1.5f);
}

void ajustesFabrica() {
  aj.a = coefTeorico();
  aj.n = 1.5f;
  aj.densidad = DENSIDAD_ORINA;
  aj.escala = ESCALA_DEFECTO;
}

void cargarAjustes() {
  ajustesFabrica();
  aj.a = prefs.getFloat("a", aj.a);
  aj.n = prefs.getFloat("n", aj.n);
  aj.densidad = prefs.getFloat("dens", aj.densidad);
  aj.escala = prefs.getFloat("esc", aj.escala);
}

void guardarAjustes() {
  prefs.putFloat("a", aj.a);
  prefs.putFloat("n", aj.n);
  prefs.putFloat("dens", aj.densidad);
  prefs.putFloat("esc", aj.escala);
}

// ----------------------------------------------------------------- sensor ----
// Devuelve la lectura en mm de columna de agua (sin restar el cero).

#if defined(SENSOR_MS4525DO)
bool leerSensor(float &hw) {
  if (Wire.requestFrom((uint8_t)MS4525_DIR, (uint8_t)4) != 4) return false;
  uint8_t b0 = Wire.read(), b1 = Wire.read();
  Wire.read(); Wire.read();                     // temperatura: no se usa
  if ((b0 >> 6) == 3) return false;             // estado 3 = fallo
  uint16_t raw = ((uint16_t)(b0 & 0x3F) << 8) | b1;
  float psi = ((float)raw - 1638.3f) * 2.0f / 13106.4f - 1.0f;   // salida tipo A, ±1 psi
  hw = PRESION_SIGNO * psi * 6894.757f / 9.80665f * aj.escala;   // Pa -> mm de agua
  return true;
}
#elif defined(SENSOR_HX710B)
portMUX_TYPE muxHx = portMUX_INITIALIZER_UNLOCKED;
bool leerSensor(float &hw) {
  if (digitalRead(PIN_HX_OUT) == HIGH) return false;   // conversión aún no lista
  int32_t v = 0;
  portENTER_CRITICAL(&muxHx);                          // SCK alto > 60 µs apaga el HX710B
  for (int i = 0; i < 24; i++) {
    digitalWrite(PIN_HX_SCK, HIGH); delayMicroseconds(1);
    v = (v << 1) | digitalRead(PIN_HX_OUT);
    digitalWrite(PIN_HX_SCK, LOW); delayMicroseconds(1);
  }
  for (int i = 0; i < 3; i++) {                        // 27 pulsos: diferencial a 40 Hz
    digitalWrite(PIN_HX_SCK, HIGH); delayMicroseconds(1);
    digitalWrite(PIN_HX_SCK, LOW); delayMicroseconds(1);
  }
  portEXIT_CRITICAL(&muxHx);
  if (v & 0x800000) v |= 0xFF000000;                   // extensión de signo
  hw = PRESION_SIGNO * (float)v * aj.escala;
  return true;
}
#endif

// ----------------------------------------------------------------- estado ----
Estado estado = LISTO;
Medida med, ultima;
bool hayUltima = false;

float cero = 0;                 // lectura del sensor con la cámara vacía
float bufMedia[MEDIA_MUESTRAS];
int iMedia = 0, nMedia = 0;
float hFilt = 0;                // nivel filtrado (mm de columna de agua)
float qAct = 0;                 // caudal actual (mL/s)
float hist1s[FS_HZ];            // nivel de hace 1 s, para detectar el arranque
int iHist = 0;

float tMedida = 0, tCalma = 0, tUltFlujo = 0;
float anillo[20];               // bloques de 0,1 s para la media de 2 s
int iAnillo = 0, nAnillo = 0, nBloques = 0;
float acumBloque = 0, tBloque = 0;

const int CURVA_N = 600;        // curva de caudal a 5 Hz (2 min)
float curva[CURVA_N];
int nCurva = 0;

// Sumas ∫h^n dt para varios exponentes: permiten calibrar a y n con vertidos de agua.
const float N_GRID[NGRID] = {1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 1.9f};
float sumaN[NGRID];

const int MAX_VERTIDOS = 10;
Vertido vertidos[MAX_VERTIDOS];
int nVertidos = 0;

uint32_t tResultadoMs = 0, tActividadMs = 0, tPantallaMs = 0, tBateriaMs = 0, tTrazaMs = 0;
float vBateria = 4.0f;
bool traza = false;

// ---------------------------------------------------------------- medida ----
void reiniciarFiltro() {
  nMedia = 0; iMedia = 0;
  for (int i = 0; i < FS_HZ; i++) hist1s[i] = 0;
}

void ajustarCero(uint16_t ms) {
  float s = 0; int n = 0;
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    float hw;
    if (leerSensor(hw)) { s += hw; n++; }
    delay(5);
  }
  if (n) cero = s / n;
  reiniciarFiltro();
}

void empezarMedida() {
  estado = MIDIENDO;
  memset(&med, 0, sizeof(med));
  tMedida = 0; tCalma = 0; tUltFlujo = 0;
  iAnillo = nAnillo = nBloques = 0; acumBloque = tBloque = 0;
  nCurva = 0;
  for (int k = 0; k < NGRID; k++) sumaN[k] = 0;
  Serial.println(F("# inicio de medida"));
  Serial.println(F("t_s,h_mm,q_mLs,v_mL"));
}

void terminarMedida() {
  med.tMiccion = tUltFlujo;
  med.qmed = med.tFlujo > 0 ? med.vol / med.tFlujo : 0;
  tActividadMs = millis();
  if (med.vol < V_MIN_GUARDAR) {
    Serial.printf("# descartada: %.1f mL (< %.0f mL)\n", med.vol, V_MIN_GUARDAR);
    estado = LISTO;
    return;
  }
  ultima = med;
  hayUltima = true;
  guardarHistorial(ultima);
  Serial.printf("# resultado: V=%.1f mL  Qmax=%.1f mL/s  Qmed=%.1f mL/s  t=%.1f s  tQmax=%.1f s%s%s\n",
                ultima.vol, ultima.qmax, ultima.qmed, ultima.tMiccion, ultima.tQmax,
                ultima.aviso ? "  [cerca del tope]" : "", ultima.rebose ? "  [REBOSE: no valida]" : "");
  estado = RESULTADO;
  tResultadoMs = millis();
}

void acumularMedida(float hBruto, float dt) {
  tMedida += dt;
  med.vol += qAct * dt;

  float hw = max(0.0f, hFilt);                    // agua equivalente, sin densidad
  if (hw > H_MIN_Q_MM)
    for (int k = 0; k < NGRID; k++) sumaN[k] += powf(hw, N_GRID[k]) * dt;

  if (qAct > Q_FLUJO_MIN) { med.tFlujo += dt; tUltFlujo = tMedida; }
  if (hFilt > RANURA_ALTO_MM - 5) med.aviso = true;
  if (hBruto > REBOSE_MM) med.rebose = true;          // sin filtrar: la media recorta picos

  // Qmax: bloques de 0,1 s y media de los 20 últimos (2 s)
  acumBloque += qAct * dt; tBloque += dt;
  if (tBloque >= 0.1f) {
    float qb = acumBloque / tBloque;
    acumBloque = 0; tBloque = 0;
    anillo[iAnillo] = qb; iAnillo = (iAnillo + 1) % 20;
    if (nAnillo < 20) nAnillo++;
    if (nAnillo == 20) {
      float m = 0;
      for (int i = 0; i < 20; i++) m += anillo[i];
      m /= 20;
      if (m > med.qmax) { med.qmax = m; med.tQmax = tMedida - 1.0f; }
    }
    if ((nBloques++ & 1) == 0 && nCurva < CURVA_N) curva[nCurva++] = qb;
    Serial.printf("%.1f,%.2f,%.2f,%.1f\n", tMedida, hFilt, qAct, med.vol);
  }

  tCalma = hFilt < H_FIN_MM ? tCalma + dt : 0;
  if (tCalma >= T_FIN_S || tMedida > T_MAX_MEDIDA_S) terminarMedida();
}

void procesarMuestra(float hw, float dt) {
  bufMedia[iMedia] = hw - cero;
  iMedia = (iMedia + 1) % MEDIA_MUESTRAS;
  if (nMedia < MEDIA_MUESTRAS) nMedia++;
  float s = 0;
  for (int i = 0; i < nMedia; i++) s += bufMedia[i];
  hFilt = s / nMedia;

  float subida = hFilt - hist1s[iHist];
  hist1s[iHist] = hFilt;
  iHist = (iHist + 1) % FS_HZ;

  float hu = max(0.0f, hFilt) / aj.densidad;      // altura real con la densidad de la orina
  qAct = hu > H_MIN_Q_MM ? aj.a * powf(hu, aj.n) : 0;

  switch (estado) {
    case LISTO:
      // El cero sigue despacio la deriva térmica mientras la cámara está vacía y quieta
      if (fabsf(subida) < 0.3f && hFilt < 5.0f) cero += 0.01f * (hw - cero);
      if (hFilt > H_INICIO_MM && subida > SUBIDA_INICIO_MM) empezarMedida();
      break;
    case MIDIENDO:
      acumularMedida(hw - cero, dt);
      break;
    case RESULTADO:
      // Se ignora el flujo: así se puede enjuagar con agua sin registrar nada
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
  h[cuenta++] = { num, m.vol, m.qmax, m.qmed, m.tMiccion, (uint8_t)((m.rebose ? 1 : 0) | (m.aviso ? 2 : 0)) };
  prefs.putBytes("hist", h, sizeof(Registro) * cuenta);
}

void imprimirHistorial() {
  Registro h[HISTORIAL_N];
  int cuenta = leerHistorial(h);
  Serial.println(F("num,vol_mL,qmax_mLs,qmed_mLs,t_s,notas"));
  for (int i = 0; i < cuenta; i++)
    Serial.printf("%lu,%.1f,%.1f,%.1f,%.1f,%s\n", (unsigned long)h[i].num, h[i].vol, h[i].qmax, h[i].qmed,
                  h[i].tMiccion, (h[i].flags & 1) ? "rebose" : (h[i].flags & 2) ? "cerca del tope" : "");
}

// ------------------------------------------------------------- pantalla ----
void pintarBateria() {
  float f = constrain((vBateria - 3.3f) / 0.9f, 0.0f, 1.0f);
  oled.drawRect(112, 1, 13, 7, SSD1306_WHITE);
  oled.fillRect(125, 3, 2, 3, SSD1306_WHITE);
  oled.fillRect(114, 3, (int)(f * 9 + 0.5f), 3, SSD1306_WHITE);
}

void pantallaListo() {
  oled.setTextSize(1); oled.setCursor(0, 0); oled.print(F("Uroflujometro"));
  pintarBateria();
  oled.setTextSize(2); oled.setCursor(0, 18); oled.print(F("LISTO"));
  oled.setTextSize(1); oled.setCursor(0, 42); oled.print(F("Orina en el embudo"));
  oled.setCursor(0, 56); oled.printf("nivel %.1f mm", hFilt);
}

void pantallaMidiendo() {
  oled.setTextSize(1); oled.setCursor(0, 0); oled.print(F("MIDIENDO"));
  pintarBateria();
  if (med.aviso && (millis() / 300) % 2) { oled.setCursor(64, 0); oled.print(F("LLENO!")); }
  oled.setTextSize(3); oled.setCursor(0, 16); oled.print((int)roundf(med.vol));
  oled.setTextSize(1); oled.print(F(" mL"));
  oled.setCursor(0, 56); oled.printf("%.1f mL/s", qAct);
  // Barra de nivel frente al rebose interno
  const int alto = 46;
  int lleno = (int)(constrain(hFilt / REBOSE_MM, 0.0f, 1.0f) * alto);
  oled.drawRect(119, 14, 8, alto + 2, SSD1306_WHITE);
  oled.fillRect(121, 15 + alto - lleno, 4, lleno, SSD1306_WHITE);
}

void pantallaResultado() {
  oled.setTextSize(2); oled.setCursor(0, 0); oled.print((int)roundf(ultima.vol));
  oled.setTextSize(1); oled.print(F(" mL"));
  pintarBateria();
  oled.setCursor(0, 19); oled.printf("Qmax %.1f Qmed %.1f", ultima.qmax, ultima.qmed);
  oled.setCursor(0, 29); oled.printf("t %.0f s", ultima.tMiccion);
  if (ultima.rebose) oled.print(F("  REBOSE!"));
  else if (ultima.vol < 150) oled.print(F("  vol<150"));
  // Curva de caudal
  float qm = 1;
  for (int i = 0; i < nCurva; i++) qm = max(qm, curva[i]);
  int yAnt = 63;
  for (int x = 0; x < 128 && nCurva > 1; x++) {
    int j = (long)x * nCurva / 128;
    int y = 63 - (int)(curva[j] / qm * 24);
    if (x) oled.drawLine(x - 1, yAnt, x, y, SSD1306_WHITE);
    yAnt = y;
  }
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
  oled.setTextSize(2); oled.setCursor(0, 12); oled.print(l1);
  oled.setTextSize(1); oled.setCursor(0, 40); oled.print(l2);
  oled.display();
}

// ------------------------------------------------------- energía y botón ----
float leerBateria() {
  return analogReadMilliVolts(PIN_BATERIA) * 2.0f / 1000.0f;   // divisor 1:2
}

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
  // Durante la medida se ignora, para no cortarla por accidente
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
  Serial.printf("Guardado: Q = %.5f * h^%.1f\n", aj.a, aj.n);
  for (int i = 0; i < nVertidos; i++) {
    float v = aj.a * vertidos[i].s[mejor];
    Serial.printf("  vertido %d: real %.1f mL, modelo %.1f mL (%+.1f %%)\n", i + 1, vertidos[i].v, v, (v - vertidos[i].v) / vertidos[i].v * 100);
  }
  if (nVertidos == 1) Serial.println(F("Con un solo vertido solo se ajusta 'a'. Haz al menos 3 a ritmos distintos."));
}

void imprimirEstado() {
#if defined(SENSOR_MS4525DO)
  Serial.println(F("Sensor: MS4525DO"));
#else
  Serial.println(F("Sensor: HX710B"));
#endif
  Serial.printf("Q = %.5f * h^%.2f  (teorico: %.5f * h^1.5)\n", aj.a, aj.n, coefTeorico());
  Serial.printf("densidad %.3f  escala %.6f  cero %.3f\n", aj.densidad, aj.escala, cero);
  Serial.printf("nivel %.2f mm  caudal %.2f mL/s  bateria %.2f V\n", hFilt, qAct, vBateria);
  Serial.printf("vertidos registrados: %d\n", nVertidos);
}

void comando(String c) {
  c.trim();
  if (!c.length()) return;
  int esp = c.indexOf(' ');
  String cmd = esp < 0 ? c : c.substring(0, esp);
  float arg = esp < 0 ? NAN : c.substring(esp + 1).toFloat();
  cmd.toLowerCase();

  if (cmd == "ayuda") {
    Serial.println(F("cero            ajusta el cero (camara vacia)"));
    Serial.println(F("estado          muestra parametros y lectura actual"));
    Serial.println(F("traza           activa/desactiva la salida continua del nivel"));
    Serial.println(F("escala <mm>     calibra el sensor con la camara llena de agua a <mm> (ranura tapada)"));
    Serial.println(F("cal <mL>        registra la ultima medida como vertido de agua de <mL> conocidos"));
    Serial.println(F("ajustar         calcula a y n con los vertidos registrados"));
    Serial.println(F("borrarcal       borra los vertidos registrados"));
    Serial.println(F("densidad <d>    densidad relativa de la orina (1.000-1.040)"));
    Serial.println(F("historial       ultimas medidas"));
    Serial.println(F("fabrica         vuelve a los valores teoricos"));
  } else if (cmd == "cero") {
    ajustarCero(1000);
    Serial.printf("Cero = %.3f\n", cero);
  } else if (cmd == "estado") {
    imprimirEstado();
  } else if (cmd == "traza") {
    traza = !traza;
    Serial.println(traza ? F("traza activada (h_mm, q_mLs)") : F("traza desactivada"));
  } else if (cmd == "escala") {
    if (isnan(arg) || arg < 10) { Serial.println(F("Uso: escala <mm>, con al menos 10 mm de agua")); return; }
    if (hFilt < 5) { Serial.println(F("La camara parece vacia: tapa la ranura y llenala de agua")); return; }
    float f = arg / hFilt;
    aj.escala *= f;
    cero *= f;
    reiniciarFiltro();
    guardarAjustes();
    Serial.printf("Escala corregida x%.4f -> %.6f\n", f, aj.escala);
  } else if (cmd == "cal") {
    if (isnan(arg) || arg <= 0) { Serial.println(F("Uso: cal <mL>  (gramos de agua vertidos)")); return; }
    if (!hayUltima) { Serial.println(F("No hay ninguna medida terminada")); return; }
    if (nVertidos >= MAX_VERTIDOS) { Serial.println(F("Lista llena: usa borrarcal")); return; }
    vertidos[nVertidos].v = arg;
    memcpy(vertidos[nVertidos].s, sumaN, sizeof(sumaN));
    nVertidos++;
    float comoAgua = aj.a * sumaN[indiceN(aj.n)];
    Serial.printf("Vertido %d: real %.1f mL, medido como agua %.1f mL (%+.1f %%). Qmax %.1f mL/s\n",
                  nVertidos, arg, comoAgua, (comoAgua - arg) / arg * 100, ultima.qmax);
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
#if defined(SENSOR_HX710B)
  pinMode(PIN_HX_SCK, OUTPUT);
  digitalWrite(PIN_HX_SCK, LOW);
  pinMode(PIN_HX_OUT, INPUT);
#endif
  analogReadResolution(12);
  Wire.begin();
  Wire.setClock(400000);
  prefs.begin("uroflujo", false);
  cargarAjustes();

  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_DIR)) Serial.println(F("! No se encuentra la pantalla OLED"));
  mensaje("Uroflujo", "Camara vacia: cero...");
  while (digitalRead(PIN_BOTON) == LOW) delay(10);   // soltar el botón tras despertar
  delay(300);                                         // el sensor se estabiliza

  float prueba;
  if (!leerSensor(prueba)) {
    delay(100);
    if (!leerSensor(prueba)) Serial.println(F("! El sensor de presion no responde"));
  }
  ajustarCero(1000);
  vBateria = leerBateria();
  tActividadMs = millis();
  Serial.println(F("Uroflujometro listo. Escribe 'ayuda' para ver los comandos."));
}

void loop() {
  uint32_t ahora = millis();
  static uint32_t tPrev = 0;

  // Muestreo: el MS4525DO se lee a FS_HZ; el HX710B avisa cuando tiene dato
  bool toca = true;
#if defined(SENSOR_MS4525DO)
  toca = (ahora - tPrev) >= (1000 / FS_HZ);
#endif
  float hw;
  if (toca && leerSensor(hw)) {
    float dt = tPrev ? (ahora - tPrev) / 1000.0f : 1.0f / FS_HZ;
    if (dt > 0.2f) dt = 0.2f;
    tPrev = ahora;
    procesarMuestra(hw, dt);
  }

  leerBoton();
  leerSerie();

  if (traza && estado != MIDIENDO && ahora - tTrazaMs >= 100) {
    tTrazaMs = ahora;
    Serial.printf("%.2f,%.2f\n", hFilt, qAct);
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
