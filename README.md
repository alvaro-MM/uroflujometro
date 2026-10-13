# Medidor de volumen de orina · prototipos

Mide el volumen de cada micción, sin depósito, para un **balance de líquidos** (lo que se bebe frente a lo que se orina). El objetivo de precisión es de **±10 mL por micción**. Está pensado para un hombre autónomo que orina de pie y también sentado.

## Plan

| Dónde | Solución | Estado |
|---|---|---|
| Casa | Recolector graduado tipo «sombrero» y báscula de cocina de 1 g, comprados | Lista de compra abajo |
| Fuera de casa | **Uroflujómetro portátil v0.2**: se sujeta con la mano, la orina entra, se mide y sale por la boquilla al inodoro o al urinario | Prototipo en esta carpeta |
| Referencia | v0.1: fijo al inodoro y mide también el caudal | Al final de este documento |

## Versiones, modelos 3D y vídeo

**El simulador hace de control de versiones.** Su sección «Versiones del diseño» muestra:
- la línea de versiones (v0.1 → v0.2) con la fecha y el estado de cada una;
- qué cambió en cada versión y por qué;
- una tabla comparativa que marca lo que cambia respecto a la versión anterior;
- un visor 3D para girar, despiezar y cortar cada modelo, o compararlos a la misma escala;
- un botón que lleva esa versión al banco de ensayo.

Los datos están en [simulador/versiones.js](simulador/versiones.js). Cada cambio del aparato añade ahí una entrada nueva y actualiza el resto de documentos.

**Modelos 3D:** [simulador/modelos3d.js](simulador/modelos3d.js) reproduce la geometría de los dos `.scad` con three.js. Lo usan el visor y el vídeo.

**Vídeo:** [video/modelos-3d.webm](video/modelos-3d.webm) dura 45 s, a 1280×720 y 30 fps:
- las dos versiones girando a la misma escala;
- de cada versión, una vista general de 360°, el despiece y un corte por el eje con el agua dentro.

Se abre con el navegador o con el Reproductor multimedia de Windows. Para volver a generarlo tras un cambio (hace falta Edge o Chrome y tarda unos 2 minutos):

```
node video/grabar_video.js
```

Lo dibuja [video/grabar.html](video/grabar.html); abierto con `?t=12` muestra el fotograma del segundo 12.

## Casa: sin fabricar nada

- **Recolector de orina tipo «sombrero» graduado** (farmacia, ortopedia o internet, 3–8 €). Se pone bajo el asiento, en la parte delantera de la taza. Sentado, recoge la orina y las heces caen detrás. De pie, se levanta el asiento y se apunta al recolector.
- **Báscula de cocina digital de 1 g**, hasta 2–5 kg (10–15 €).
- **Uso:** tara con el recolector vacío, orina, pesa y apunta (1 g ≈ 1 mL). Después se vacía en el inodoro y se enjuaga.
- **Precisión:** unos ±5 mL. La orina pesa algo más que el agua (entre un 0,5 y un 3 %): en 300 mL la báscula marca 2–9 g de más. Para afinar, restad un 1,5 %.

## Fuera de casa: uroflujómetro portátil v0.2

### Cómo funciona

```
         embudo acodado (30°)
                 │
        ┌────────▼────────┐  respiradero
        │ ┌──┐ buzo ┌──┐  │
        │ │  │ ┃  ┃ │  │  │   cámara Ø24 con 4 ranuras
        │ │~~│~┃~~┃~│~~│  │   de 1,5 × 36 mm alrededor
        │ │  │ ┗┳┳┛ │  │  │ ← el tapón del buzo reparte
        │ │  └──●───┘  │  │   el chorro hacia los lados
        │ │   toma     │  │ ← camisa: lo que sale por las
        │ └────┬───────┘  │   ranuras baja hasta la boquilla
        └──────┼─────┬────┘
   columna de aire   ▼ boquilla al inodoro / urinario
               │
   sensor de presión + acelerómetro (en la caja)
```

1. **Embudo acodado:** el codo de 30° permite usarlo de pie y sentado.
2. **Buzo con tapón:** la orina baja por el tubo central y sale por cuatro ventanas laterales orientadas entre las ranuras. Así el chorro no golpea la toma de presión.
3. **Cámara de Ø24 mm con cuatro ranuras de 1,5 × 36 mm:** el nivel sube hasta que lo que sale iguala lo que entra.
4. **Sensor de presión (MS4525DO):** mide la profundidad en el centro del suelo a través de una columna de aire, sin tocar la orina.
5. **Acelerómetro (LIS3DH):** mide la inclinación y el movimiento de la mano. Con eso, el firmware calcula cuánta agua tiene encima cada ranura: `Q = Σ a/4 · √g · dᵢⁿ / cos αᵢ`.
6. **Camisa anular:** lo que sale por las ranuras cae por ella hasta una boquilla central que va al inodoro.

Mide unos 4 × 6 × 8 cm sin el embudo (unos 16 cm con él) y pesa 60–80 g.

Con las ranuras llenas salen 72 mL/s, más que el chorro más fuerte previsto (50 mL/s). Por encima, a 40 mm, hay ventanas de rebose interno: nada cae fuera y la medida se marca como no válida.

### Lo que dice el simulador

En el [simulador](simulador/index.html), elige «Compacto portátil (v0.2)». Resultados con 300 mL y un chorro normal:

| Situación | Error |
|---|---|
| Recto | < 1 mL |
| Inclinado 15° con balanceo de ±5° | unos 2 mL |
| Inclinado 25° | unos 3 mL |
| Inclinado 20° sin corrección del acelerómetro | unos −20 mL |
| Error de calibración del 2 % | suma unos 6 mL |

**Conclusión:** los ±10 mL son alcanzables en micciones normales si la calibración queda por debajo del 2 %. En micciones grandes (500 mL), cada 1 % de calibración son 5 mL. El simulador no incluye el oleaje del líquido al mover el aparato bruscamente.

### Contenido

| Ruta | Qué es |
|---|---|
| [simulador/](simulador/index.html) | Banco de ensayo virtual y control de versiones (v0.2 y v0.1) |
| [simulador/versiones.js](simulador/versiones.js) | Registro de versiones: cambios, motivos y características |
| [simulador/modelos3d.js](simulador/modelos3d.js) | Modelos 3D para el visor y el vídeo |
| [video/modelos-3d.webm](video/modelos-3d.webm) | Vídeo de los dos modelos (se regenera con `node video/grabar_video.js`) |
| [firmware/uroflujometro_v02/](firmware/uroflujometro_v02/) | Programa para la XIAO ESP32-C3 |
| [cad/uroflujometro_v02.scad](cad/uroflujometro_v02.scad) | Modelo 3D de las piezas |
| [infografia/uroflujometro-infografia-v02.png](infografia/uroflujometro-infografia-v02.png) | Infografía de la v0.2 (se edita en `infografia_v02.html`) |
| [infografia/uroflujometro-infografia-v01.png](infografia/uroflujometro-infografia-v01.png) | Infografía de la v0.1, como referencia |

### Materiales

| Pieza | Referencia de ejemplo | Precio aprox. |
|---|---|---|
| Microcontrolador | Seeed Studio XIAO ESP32-C3 | 5–7 € |
| Sensor de presión (obligatorio en la v0.2) | MS4525DO ±1 psi, I2C, **versión 3,3 V** (referencia con «DS3») | 15–30 € |
| Acelerómetro | Placa LIS3DH I2C | 2–5 € |
| Pantalla | OLED 0,91" 128×32 I2C (o 0,96" 128×64 cambiando `OLED_ALTO`) | 2–4 € |
| Batería | LiPo 3,7 V 200–300 mAh (por ejemplo, 402030) | 3–5 € |
| Pulsador y resistencias | Pulsador de panel Ø7; 2 × 220 kΩ y 1 × 10 kΩ | < 1 € |
| Toma de presión | Tubito de latón de 3 mm × 8 mm (o aguja roma 14G) y 6 cm de tubo de silicona de 3 mm interior | 2 € |
| Filamento | PETG, unos 60 g; TPU opcional para el embudo | 2 € |
| Para llevarlo | Bolsa estanca con cierre, que quepa en la riñonera | 1 € |

Total: unos 35–55 €.

### Conexiones

| XIAO ESP32-C3 | Va a |
|---|---|
| 3V3 | VCC de la OLED |
| D7 (GPIO20) | VCC del MS4525DO y del LIS3DH (se apagan al dormir) |
| GND | GND de todo |
| D4 (SDA) / D5 (SCL) | SDA / SCL de OLED, MS4525DO y LIS3DH |
| D1 (GPIO3) | Pulsador a GND + 10 kΩ de D1 a 3V3 |
| A0 (GPIO2) | Punto medio del divisor 220 k / 220 k entre BAT+ y GND |
| Pads BAT+ / BAT− | Batería LiPo |

**Orientación del acelerómetro.** El firmware necesita saber hacia dónde mira cada eje. La configuración por defecto (`MAPA_ACC` en `config.h`) supone la placa pegada en vertical a la pared frontal de la caja: su eje Y hacia arriba, el X hacia la derecha mirando la pantalla y el Z hacia ti. Si la montas de otra forma, cambia `MAPA_ACC`.

### Impresión

Abre `cad/uroflujometro_v02.scad`, elige `pieza`, pulsa F6 y exporta el STL. Las piezas ya salen orientadas para imprimir.

| Pieza | Notas |
|---|---|
| `cuerpo` | Boca abajo, sin soportes. Unos 44 × 57 × 76 mm |
| `tapa` | Plana |
| `buzo` | Con la pestaña abajo |
| `caja_tapa` | Con la placa abajo |
| `embudo` | Con el caño abajo y soportes bajo la boca, o en TPU |

- **Las ranuras de 1,5 mm son lo más delicado.** Imprime con capas de 0,12–0,16 mm, comprueba que entra una broca de 1,5 mm y quita las rebabas sin lijar el interior.
- **Toma de presión:** pega el tubito de latón con epoxi en el agujero de Ø3 que hay dentro de la caja.
- Si cambias la geometría, cambia también los valores de `config.h`.

### Montaje

1. Conecta el tubo de silicona del tubito de latón al puerto del MS4525DO que da lectura positiva. El sensor va en la parte alta de la caja.
2. Coloca la electrónica en la caja, la OLED detrás de la ventana y el botón en su agujero.
3. Mete el buzo por el cuello de la tapa con la chaveta en su ranura. Pon la tapa con la muesca alineada con la marca del cuerpo: así las ventanas del buzo quedan entre las ranuras.
4. Encaja el embudo en el cuello. Gira libremente: oriéntalo para que la boca mire hacia quien lo usa.

### Programación

Arduino IDE 2 con el núcleo esp32. Placa **XIAO_ESP32C3**, con **USB CDC On Boot** activado. Librerías: **Adafruit SSD1306** y **Adafruit GFX**. Revisa `OLED_ALTO` y `MAPA_ACC` en `config.h` y sube `uroflujometro_v02.ino`. En el monitor serie (115200 baudios), escribe `ayuda`.

### Calibración (imprescindible)

1. **Nivel:** con el aparato recto sobre una mesa (compruébalo con un nivel de burbuja sobre la tapa), escribe `nivel`.
2. **Comprobar los ejes con `estado`:**
   - De pie, `arriba` debe salir ≈ (0, 0, 1).
   - Tumbado con la ranura 1 (+X, a la derecha de la pantalla) hacia abajo, ≈ (−1, 0, 0).
   - Tumbado con la pantalla hacia abajo, ≈ (0, 1, 0).

   Si no sale así, corrige `MAPA_ACC`.
3. **Vertidos:** pesa unos 250 g de agua en un vaso y viértelos en el embudo. Cuando salga el resultado, escribe `cal 250` con los gramos reales. Haz **al menos cuatro vertidos**: uno rápido con el aparato recto, uno lento, uno inclinado unos 15° y uno moviendo un poco la mano.
4. Escribe `ajustar`. El firmware elige el exponente que da menos dispersión y guarda la ley.
5. **Comprobación:** haz un vertido nuevo. El objetivo es menos de 5 mL de error con agua.
6. La densidad de la orina viene a 1,015. Se cambia con `densidad 1.020`.

### Uso

1. Enciende con el botón y espera a «LISTO» con el aparato quieto y la cámara vacía.
2. Sujétalo por la caja, con el embudo hacia ti y la boquilla hacia el inodoro o el urinario, de pie o sentado. Intenta no pasar de 25° de inclinación: si te pasas, la pantalla avisa con «INCLINADO».
3. Orina. La medida empieza sola y el volumen sube en la pantalla.
4. A los 4 s de la última gota queda el resultado en grande durante 3 minutos. Apúntalo. Si pone «DUDOSA» (se inclinó demasiado) o «REBOSE», anótalo también.
5. Sacúdelo, enjuágalo si hay lavabo y guárdalo en la bolsa estanca. Pulsa el botón para preparar otra medida; una pulsación larga lo apaga.

Si se olvida apuntar alguna medida, el aparato guarda las últimas 30. Se consultan con `historial` por el monitor serie.

### Limpieza

En casa, desmonta la tapa, el buzo y el embudo. Lávalos con agua tibia y jabón y pasa un cepillo fino por las ranuras. Si una ranura se obstruye, la medida cambia: recalibra si notas diferencias.

### Límites

- **Sin probar en hardware:** el firmware no se ha compilado y el modelo 3D no se ha impreso. Los ±10 mL son un objetivo que hay que confirmar con la calibración.
- **Depende del acelerómetro:** si falta, la pantalla muestra «SIN ACELEROMETRO» y el aparato mide como si estuviera recto, con mucho error al inclinarlo.
- **Las ranuras de 1,5 mm** son sensibles a la calidad de impresión y a la suciedad.
- **No es un producto sanitario.**

---

## Versión 0.1 · fija al inodoro, mide también el caudal

Mide el volumen de orina (50–800 mL) y, de paso, el caudal máximo y medio. La orina entra por el embudo, pasa por el aparato y sale por un tubo al inodoro. Firmware en `firmware/uroflujometro/` y modelo en `cad/uroflujometro.scad`.

### Por qué no se monta tal cual el diseño de la imagen

| Problema | Consecuencia |
|---|---|
| El SFM3000 de Sensirion mide **gases** (respiradores), no líquidos | No sirve para orina |
| Rango de 150 mL/min = **2,5 mL/s**; un chorro normal va de 15 a 30 mL/s y puede pasar de 40 | Satura en el primer segundo |
| Un sensor en línea con canal estrecho frena el paso | **El embudo se llena en 3–6 s y desborda** |
| Cuerpo horizontal de 2 cm | Sin altura de líquido no hay fuerza para evacuar el chorro |

El simulador de la carpeta `simulador/` lo muestra con números: con el diseño original se derraman unos 170 de 300 mL.

### Principio del prototipo

```
          embudo
            │
      ┌─────▼─────┐   respiradero
      │  ┌──┐     │ ┌──┐
      │  │  │buzo │ │  │ canal de descarga
      │  │  │     ┆ │  │ (a presión atmosférica)
      │~~│~~│~~~~~┆→│  │  ← la ranura vierte aquí
      │  │  │  h  ┆ │  │
      │  └──┘     ┆ │  │
      └──●────────┘ └┬─┘
         │ toma       │
   columna de aire    ▼ salida Ø12 al inodoro
         │
   sensor de presión
```

1. La orina cae por un tubo buzo hasta el fondo de una cámara de Ø32 mm. El buzo frena el chorro para que no agite la superficie.
2. La cámara solo puede vaciarse por una **ranura vertical de 3 × 70 mm**. A más caudal, más sube el nivel `h`, y la ranura deja salir `Q = ⅔·Cd·b·√(2g)·h^1,5`.
3. Un sensor de presión mide `h` a través de una columna de aire atrapada en un tubo: el sensor no toca la orina.
4. El firmware integra `V = ∫Q dt` y calcula Qmax con la media móvil de 2 s que recomienda la ICS.

**Por qué no desborda:** la ranura llena evacua unos 100 mL/s, el doble del chorro más fuerte previsto (50 mL/s). Por encima de la ranura hay ventanas de rebose interno a 78 mm que llevan el exceso al canal de descarga: nada cae fuera, y el firmware marca esa medida como no válida. El embudo de 220 mL queda como reserva adicional.

### Contenido

| Ruta | Qué es |
|---|---|
| [simulador/index.html](simulador/index.html) | Banco de ensayo virtual: perfiles de micción, diseño, desborde y error |
| [simulador/sim.js](simulador/sim.js) | Modelo hidráulico + el mismo algoritmo que el firmware |
| [firmware/uroflujometro/](firmware/uroflujometro/) | Sketch de Arduino para XIAO ESP32-C3 |
| [cad/uroflujometro.scad](cad/uroflujometro.scad) | Modelo 3D paramétrico (OpenSCAD) de todas las piezas |

### Materiales

| Pieza | Referencia de ejemplo | Precio aprox. |
|---|---|---|
| Microcontrolador | Seeed Studio XIAO ESP32-C3 (carga LiPo y USB-C integrados) | 5–7 € |
| Sensor de presión (recomendado) | MS4525DO ±1 psi diferencial, I2C, **versión 3,3 V** (referencia con «DS3», p. ej. MS4525DO-DS3AI001DP) | 15–30 € |
| Sensor alternativo barato | Módulo HX710B + MPS20N0040D (0–40 kPa) | 2–3 € |
| Pantalla | OLED SSD1306 0,96" 128×64 I2C | 3–5 € |
| Batería | LiPo 3,7 V 300–500 mAh con cables | 4–6 € |
| Pulsador | De panel, Ø7 mm | < 1 € |
| Resistencias | 2 × 220 kΩ (divisor de batería), 1 × 10 kΩ (pull-up del botón) | < 1 € |
| Interruptor deslizante (opcional) | En el positivo de la batería, para guardarlo meses | < 1 € |
| Tubo de silicona | 3 mm interior, ~15 cm (toma de presión) | 2 € |
| Tubo de salida | Silicona o PVC 12 mm interior, 50–80 cm | 3 € |
| Filamento | PETG, ~130 g | 3 € |
| Bridas | 2 unidades (soporte del inodoro) | — |

Total: unos 45–60 € con el MS4525DO, unos 25 € con el HX710B. El HX710B tiene más ruido (≈ ±1 mm frente a ±0,1 mm): sirve para volumen, pero pierde precisión con chorros débiles.

### Conexiones

| XIAO ESP32-C3 | Va a |
|---|---|
| 3V3 | VCC de la OLED |
| D7 (GPIO20) | VCC del sensor de presión (el firmware lo apaga al dormir) |
| GND | GND de OLED, sensor, botón y divisor |
| D4 (SDA) | SDA de OLED y MS4525DO |
| D5 (SCL) | SCL de OLED y MS4525DO |
| D1 (GPIO3) | Pulsador a GND + 10 kΩ de D1 a 3V3 |
| A0 (GPIO2) | Punto medio del divisor 220 k / 220 k entre BAT+ y GND |
| Pads BAT+ / BAT− (cara inferior) | Batería LiPo |
| D2 / D3 | SCK / OUT del HX710B (solo si usas ese sensor) |

El tubo de silicona va de la espiga del compartimento al puerto del sensor que da lectura positiva al llenar (el otro puerto, al aire). Si sale negativa, cambia `PRESION_SIGNO` a `-1` en `config.h`. El sensor tiene que quedar **por encima de 90 mm** (por encima del rebose) para que nunca le llegue líquido: arriba del compartimento, junto a la OLED.

### Impresión 3D

Abre `cad/uroflujometro.scad`, elige `pieza` en el personalizador, pulsa F6 y exporta el STL. Todas las piezas están orientadas para imprimirse sin soportes.

| Pieza | Notas |
|---|---|
| `cuerpo` | De pie, tal cual. 4 perímetros, 20 % de relleno. Tamaño total ≈ 64 × 62 × 137 mm |
| `tapa` | Plana sobre la cama |
| `buzo` | Con la pestaña abajo |
| `caja_tapa` | Con la placa abajo |
| `embudo` | Con el caño abajo |
| `soporte` | Tumbado |

- **PETG**, no PLA: aguanta mejor la humedad y el agua caliente.
- Imprime la ranura con capas finas (0,16 mm) y quita las rebabas con cuidado: sus bordes deciden el coeficiente de descarga. No la lijes por dentro.
- La impresión FDM es algo porosa. Para uso continuado, sella el interior de la cámara y el canal con resina epoxi apta para alimentos, o imprime esas piezas en resina.
- Si cambias `ranura_b` o `ranura_h`, cambia también `RANURA_ANCHO_MM` y `RANURA_ALTO_MM` en `config.h`.

### Programación

1. Arduino IDE 2 + núcleo **esp32** de Espressif (2.x o 3.x).
2. Placa: **XIAO_ESP32C3**. En *Herramientas*, activa **USB CDC On Boot**.
3. Librerías: **Adafruit SSD1306** y **Adafruit GFX**.
4. En `config.h`, deja sin comentar el sensor que uses.
5. Sube `firmware/uroflujometro/uroflujometro.ino` y abre el monitor serie a 115200 baudios. Escribe `ayuda`.

### Calibración (imprescindible)

Necesitas una báscula de cocina y agua. 1 g de agua = 1 mL.

1. **Cero.** Se hace solo al encender, con la cámara vacía. Para repetirlo, pulsa el botón en la pantalla «LISTO» o escribe `cero`.
2. **Escala (solo HX710B).** Tapa la ranura por fuera con cinta americana, llena la cámara con agua hasta 50 mm medidos con una regla desde el suelo, escribe `escala 50` y quita la cinta.
3. **Vertidos.** Pesa unos 250 g de agua en un vaso y viértela en el embudo. Cuando salga el resultado, escribe `cal 250` con los gramos reales (pesa el vaso antes y después). Repite **al menos tres veces a ritmos distintos**: rápido (≈ 5 s), medio (≈ 15 s) y lento con un chorro fino (≈ 40 s).
4. Escribe `ajustar`. El firmware prueba exponentes de 1,1 a 1,9, se queda con el que da menos dispersión entre vertidos y guarda la ley `Q = a·h^n`.
5. Comprueba con un vertido nuevo. Objetivo: error menor del 3 % con agua.
6. La densidad de la orina viene a 1,015. Si la conoces (tira reactiva), ajústala con `densidad 1.020`. Una diferencia de 0,015 supone ≈ 2 % de error.

Recalibra si limpias a fondo la ranura o si notas deriva: los depósitos de sales cambian su comportamiento.

### Uso

1. Cuelga el aparato del borde del inodoro con el soporte, **en vertical**, con el tubo de salida bajando hacia la taza sin bucles (un bucle con agua inunda el canal y falsea la medida).
2. Enciende con el botón. Espera a «LISTO» sin mover el aparato: está ajustando el cero.
3. Orina en el embudo. La medida empieza sola al subir el nivel.
4. Cinco segundos después de la última gota aparece el resultado: volumen, Qmax, Qmed, duración y la curva de caudal.
5. Enjuaga con agua mientras se ve el resultado: no se registra. Pulsa el botón para preparar una medida nueva.
6. Pulsación larga (1,5 s): apagar. Se apaga solo tras 5 minutos sin uso.

Avisos en pantalla: `LLENO!` durante la medida (el nivel se acerca al tope de la ranura), `REBOSE!` en el resultado (medida no válida) y `vol<150` (con menos de 150 mL la curva de caudal es poco representativa; el volumen sigue siendo válido).

El historial de las últimas 20 medidas se consulta con `historial` por el monitor serie.

### Limpieza

- Tras cada uso: enjuague con agua desde el embudo.
- Semanal: desmonta tapa y buzo, lava con agua tibia y jabón, y pasa un cepillo de biberón por la cámara. Vinagre diluido para la cal.
- Nada de lavavajillas: el PETG se deforma por encima de unos 70 °C.

### Límites de este prototipo

- No es un producto sanitario ni está validado clínicamente. Sirve para seguimiento personal, no para diagnosticar.
- El firmware y el modelo 3D no se han probado todavía en hardware real: espera ajustes en la primera impresión (tolerancias de encaje, signo del sensor, umbrales).
- La precisión depende de la calibración. El simulador supone una calibración perfecta.
- Inclinación: la toma de presión está junto a la ranura para tolerar unos pocos grados, pero conviene colgarlo recto.

### Siguientes pasos

- Bluetooth (BLE) para enviar cada medida al móvil y llevar un diario de micciones.
- Zumbador que avise de `LLENO!`.
- Acelerómetro para avisar si el aparato está inclinado.
- Reducir el tamaño y aumentar la portbilidad del aparato
- Ranura en V para ganar resolución con chorros muy débiles.

## Licencia

MIT, ver [LICENSE](LICENSE).
