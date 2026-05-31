# Proyecto: Gemelo Digital de Ascensor Inteligente mediante Instrumentación Avanzada (ACME S.A.)

**Autor:** Javier de Celis Cuevas  y Guillermo Tudela Ciudad Real
**Titulación:** Máster en Telecomunicaciones  

## Descripción General

Este proyecto implementa el control de un ascensor inteligente equipado con sensores ambientales, control de presencia, actuadores de movimiento, climatización y un sistema de instrumentación avanzada. El sistema está diseñado para operar en el entorno industrial de ACME S.A., permitiendo supervisión autónoma, control remoto, autodiagnóstico y trazabilidad de datos.

Este proyecto ha sido simulado empleando WOKWI. Este es el enlace Wokwi: https://wokwi.com/projects/465552102203386881

La cabina está controlada por una placa **Arduino Mega 2560** y cuenta con instrumentación para medir e interactuar con:

* **Temperatura y humedad relativa** mediante el sensor DHT22.
* **Luminiscencia ambiental** con un sensor LDR.
* **Detección de presencia** con un sensor ultrasónico HC-SR04.
* **Control remoto y configuración dinámica** mediante un receptor de infrarrojos (IR).
* **Trazabilidad y Data-Logging** mediante un módulo de tarjeta MicroSD por bus SPI.
* **Visualización local (HMI)** mediante una pantalla LCD 16x2 por bus I2C.

Además, implementa un sistema de control y actuación que mantiene las condiciones de la cabina y el motor en rangos óptimos de operación mediante:

* **Algoritmos de regulación ON-OFF con zona muerta** para la temperatura y la humedad.
* **Control proporcional por PWM** para el sistema de iluminación, condicionado a la detección de presencia física.
* **Modos de seguridad y eficiencia energética**, permitiendo cambiar entre un perfil estándar y un perfil *Eco* con una histéresis más amplia, así como un modo de bloqueo por mantenimiento.

---

## Análisis Detallado de Funciones

### `MoverAscensor()`
Controla el servomotor para simular el desplazamiento físico de la cabina a la planta solicitada. Utiliza un algoritmo no bloqueante basado en `millis()` para que el movimiento sea proporcional a la distancia sin interrumpir la lectura del resto de sensores.

### `controlarIR()`
Decodifica las señales del mando a distancia infrarrojo. Permite al operario realizar llamadas a plantas, ajustar dinámicamente las consignas (setpoints) de temperatura y humedad, cambiar el perfil energético (Eco/Estándar) y forzar una parada de emergencia (Modo Mantenimiento).

### `controlarIluminacion()`
Lee el valor analógico del LDR, lo mapea a un valor PWM y verifica la distancia mediante el sensor ultrasónico HC-SR04. La iluminación se activa de forma proporcional a la oscuridad únicamente si se detecta presencia en la cabina.

### `leerAmbiente()`
Obtiene la temperatura y humedad usando la librería `DHT`. Evalúa la lógica de control ON-OFF con zona muerta y activa los actuadores correspondientes (simulados mediante LEDs para calentar, enfriar, humidificar o deshumidificar). El control se desactiva automáticamente si el sistema entra en modo mantenimiento o detecta un fallo.

### `autodiagnostico()`
Supervisa la integridad de los datos leídos. Si detecta valores nulos (`NaN`) procedentes del sensor DHT22, levanta una bandera lógica de error que detiene los actuadores climáticos como medida de seguridad industrial.

### `actualizarLCD()`
Gestiona la interfaz HMI local. Refleja la planta actual, el estado de movimiento, la intensidad lumínica y las variables climáticas. Incorpora un sistema de prioridades para la visualización de alertas críticas: sobrescribe la información ambiental para mostrar "EN MANTENIMIENTO" o "ERR SENSOR" cuando es necesario.

### `telemetriaYLog()`
Gestiona las comunicaciones externas del sistema. Por un lado, envía una trama de datos estructurada en formato JSON a través de un puerto serie de hardware (`Serial1`), simulando la conexión a un bus RS-485. Por otro lado, registra un histórico continuo (`datalog.csv`) en una tarjeta MicroSD mediante el bus SPI.
