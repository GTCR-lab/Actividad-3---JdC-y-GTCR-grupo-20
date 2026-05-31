// Librerías
#include <Servo.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include <IRremote.h>

// Pines de Hardware
const int pinBoton1 = 3;
const int pinBoton2 = 4;
const int pinBoton3 = 5;
const int pinBoton4 = 6;
const int pinBoton5 = 7;

const int servoPin = 10;
const int dhtPin = 12;
const int trigPin = 8;
const int echoPin = 9;

const int ledAzul = 2;
const int ledRojo = A1;
const int ledMorado = A2;
const int ledAmarillo = A3;
const int ledLuz = 11;

// Nuevos Pines Mega
const int chipSelect = 53; // CS para MicroSD
const int irPin = 40;      // Receptor IR

// Variables Sistema Ascensor
long duracion;
float distancia;
int plantaActual = 1;
int direccionAscensor = 0;
int plantaDestino = 1;

int anguloInicio = 0;
int anguloDestino = 0;
bool enMovimiento = false;
unsigned long tiempoInicioMovimiento = 0;
unsigned long duracionMovimiento = 0;

// Variables Control Ambiental
byte estadoTemp = 0;
byte estadoHum = 0;
float setpointTemp = 25;
float zonaMuertaTemp = 2;
float setpointHum = 80;
float zonaMuertaHum = 5;
bool errorSensores = false;
bool modoMantenimiento = false;
bool modoEco = false;

// Temporizadores Telemetría
unsigned long tiempoUltimoLog = 0;
const long intervaloLog = 5000; // Enviar datos cada 5 segundos

// Objetos
#define DHTTYPE DHT22
DHT dht(dhtPin, DHTTYPE);
Servo servo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Caracteres LCD
byte flechaArriba[8] = { B00100, B01110, B10101, B00100, B00100, B00100, B00100, B00000 };
byte flechaAbajo[8] = { B00100, B00100, B00100, B00100, B10101, B01110, B00100, B00000 };

// =================================================================================
// SETUP
// =================================================================================
void setup() {
  // Inicialización de Buses Serie (Mega tiene múltiples)
  Serial.begin(9600);  // Serial 0: Para depuración y monitor serie del PC
  Serial1.begin(9600); // Serial 1: Simulación de Bus Industrial RS-485

  Serial.println("Iniciando Sistema ACME S.A...");

  // Configuración de Pines
  pinMode(pinBoton1, INPUT_PULLUP);
  pinMode(pinBoton2, INPUT_PULLUP);
  pinMode(pinBoton3, INPUT_PULLUP);
  pinMode(pinBoton4, INPUT_PULLUP);
  pinMode(pinBoton5, INPUT_PULLUP);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(ledRojo, OUTPUT);
  pinMode(ledAzul, OUTPUT);
  pinMode(ledLuz, OUTPUT);
  pinMode(ledAmarillo, OUTPUT);
  pinMode(ledMorado, OUTPUT);

  // Inicialización de Actuadores y Sensores
  servo.attach(servoPin);
  servo.write(0);
  dht.begin();
  
  // Inicialización LCD
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, flechaArriba);
  lcd.createChar(1, flechaAbajo);

  // Inicialización Receptor IR
  IrReceiver.begin(irPin, ENABLE_LED_FEEDBACK);
  
    // Inicialización Tarjeta SD
  Serial.print("Inicializando tarjeta SD... ");
  
  pinMode(chipSelect, OUTPUT); 
  delay(1000); 

  if (!SD.begin(chipSelect)) {
    Serial.println("Fallo o tarjeta no presente.");
  } else {
    Serial.println("Tarjeta inicializada OK.");
    // Escribir cabecera del CSV si el archivo no existe
    File dataFile = SD.open("datalog.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.println("Tiempo(ms),Planta,Temp(C),Hum(%),Luz(PWM),Estado");
      dataFile.close();
    }
  }
}

// =================================================================================
// LOOP PRINCIPAL
// =================================================================================
void loop() {
  
  controlarIR(); // Comprobar si hay órdenes remotas
  
  // Selección de planta física
  if (!enMovimiento) {
    if (digitalRead(pinBoton1) == LOW) {MoverAscensor(1, 0);}
    else if (digitalRead(pinBoton2) == LOW) {MoverAscensor(2, 45);}
    else if (digitalRead(pinBoton3) == LOW) {MoverAscensor(3, 90);}
    else if (digitalRead(pinBoton4) == LOW) {MoverAscensor(4, 135);}
    else if (digitalRead(pinBoton5) == LOW) {MoverAscensor(5, 180);}
  }

  // Lógica de Movimiento
  if (enMovimiento) {
    unsigned long tiempoTranscurrido = millis() - tiempoInicioMovimiento;
    if (duracionMovimiento > 0) {
      float progreso = (float)tiempoTranscurrido / duracionMovimiento;
      if (progreso > 1) progreso = 1;
      int anguloActual = anguloInicio + (anguloDestino - anguloInicio) * progreso;
      servo.write(anguloActual);
    }
    if (tiempoTranscurrido >= duracionMovimiento) {
      enMovimiento = false;
      plantaActual = plantaDestino;
      direccionAscensor = 0;
    }
  }

  // Lecturas Ambientales y Control
  int brillo;
  float temperatura, humedad;
  
  controlarIluminacion(brillo);
  leerAmbiente(temperatura, humedad);
  
  // Autodiagnóstico de Sensores
  autodiagnostico(temperatura, humedad);

  // Mostrar en HMI Local
  actualizarLCD(brillo, temperatura, humedad);
  
  // Tareas Periódicas (SD y UART)
  if (millis() - tiempoUltimoLog >= intervaloLog) {
    tiempoUltimoLog = millis();
    telemetriaYLog(brillo, temperatura, humedad);
  }

  delay(50); 
}

// =================================================================================
// FUNCIONES AUXILIARES
// =================================================================================

void MoverAscensor(int planta, int angulo) {
  if (planta == plantaDestino || enMovimiento) return;
  
  plantaDestino = planta;
  if (planta > plantaActual) { direccionAscensor = 1; }
  else if (planta < plantaActual) { direccionAscensor = -1; }
  else { direccionAscensor = 0; }

  anguloInicio = servo.read();
  anguloDestino = angulo;
  duracionMovimiento = abs(planta - plantaActual) * 1000;
  tiempoInicioMovimiento = millis();
  enMovimiento = true;
}

void controlarIR() {
  if (IrReceiver.decode()) {
    int comando = IrReceiver.decodedIRData.command;
    
    Serial.print("Comando IR recibido: ");
    Serial.println(comando);

    // 1. PARADA DE EMERGENCIA / MANTENIMIENTO (Funciona siempre)
    if (comando == 162) { // Botón POWER
      modoMantenimiento = !modoMantenimiento;
      Serial.println(modoMantenimiento ? "MODO MANTENIMIENTO ACTIVADO" : "SISTEMA OPERATIVO");
    }

    // 2. CONTROLES OPERATIVOS (Solo funcionan si no hay mantenimiento y está parado)
    if (!modoMantenimiento && !enMovimiento) {
      switch (comando) {
        // Llamadas a plantas
        case 48: MoverAscensor(1, 0); break;   // Botón 1
        case 24: MoverAscensor(2, 45); break;  // Botón 2
        case 122: MoverAscensor(3, 90); break; // Botón 3
        case 16: MoverAscensor(4, 135); break; // Botón 4
        case 56: MoverAscensor(5, 180); break; // Botón 5
        
        // Ajuste de Temperatura (Consigna)
        case 2: setpointTemp++; Serial.print("Temp Obj: "); Serial.println(setpointTemp); break; // Botón VOL+
        case 152: setpointTemp--; Serial.print("Temp Obj: "); Serial.println(setpointTemp); break; // Botón VOL-
        
        // Ajuste de Humedad (Consigna)
        case 144: setpointHum += 5; Serial.print("Hum Obj: "); Serial.println(setpointHum); break; // Botón Fast Forward o UP
        case 224: setpointHum -= 5; Serial.print("Hum Obj: "); Serial.println(setpointHum); break; // Botón Rewind o DOWN
        
        // Perfil Energético
        case 34: // Botón TEST
          modoEco = !modoEco; 
          zonaMuertaTemp = modoEco ? 5.0 : 2.0; // Cambia la histéresis
          Serial.println(modoEco ? "MODO ECO (Histéresis 5C)" : "MODO ESTÁNDAR (Histéresis 2C)");
          break;
      }
    }
    IrReceiver.resume();
  }
}
void controlarIluminacion(int &brillo) {
  int luz = analogRead(A0);
  if (luz > 600) {
    brillo = map(luz, 1015, 600, 255, 50);
  } else {
    brillo = 0;
  }
  brillo = constrain(brillo, 0, 255);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duracion = pulseIn(echoPin, HIGH);
  distancia = duracion * 0.034 / 2;
  
  if (distancia < 100 && distancia > 0) {
    analogWrite(ledLuz, brillo);
  } else {
    analogWrite(ledLuz, 0);
  }
}

void leerAmbiente(float &temperatura, float &humedad){
  temperatura = dht.readTemperature();
  humedad = dht.readHumidity();
  estadoTemp = 0;
  estadoHum = 0;

  // Si hay error o estamos en mantenimiento, apagamos todo por seguridad
  if (errorSensores || modoMantenimiento) {
    digitalWrite(ledRojo, LOW);
    digitalWrite(ledAzul, LOW);
    digitalWrite(ledMorado, LOW);
    digitalWrite(ledAmarillo, LOW);
    return; // Salimos de la función sin evaluar la climatización
  }

  // Lógica normal de TEMPERATURA
  if (!isnan(temperatura)) {
    if (temperatura < setpointTemp - zonaMuertaTemp) {
      digitalWrite(ledRojo, HIGH);
      digitalWrite(ledAzul, LOW);
      estadoTemp = 1; 
    } else if (temperatura > setpointTemp + zonaMuertaTemp) {
      digitalWrite(ledAzul, HIGH);
      digitalWrite(ledRojo, LOW);
      estadoTemp = 2; 
    } else {
      digitalWrite(ledRojo, LOW);
      digitalWrite(ledAzul, LOW);
      estadoTemp = 0; 
    }
  }

  // Lógica normal de HUMEDAD
  if (!isnan(humedad)) {
    if (humedad < setpointHum - zonaMuertaHum) {
      estadoHum = 1;
      digitalWrite(ledMorado, HIGH);
      digitalWrite(ledAmarillo, LOW);
    } else if (humedad > setpointHum + zonaMuertaHum) {
      estadoHum = 2;
      digitalWrite(ledMorado, LOW);
      digitalWrite(ledAmarillo, HIGH);
    } else {
      digitalWrite(ledMorado, LOW);
      digitalWrite(ledAmarillo, LOW);
      estadoHum = 0;
    }
  }
}

void autodiagnostico(float t, float h) {
  // Si el sensor devuelve NaN, se activa la bandera de error
  if (isnan(t) || isnan(h)) {
    errorSensores = true;
  } else {
    errorSensores = false;
  }
}

void actualizarLCD(int brillo, float temperatura, float humedad) {
  // --- PRIMERA LÍNEA ---
  lcd.setCursor(0, 0);
  lcd.print("P:");
  if (enMovimiento) {
    if (direccionAscensor == 1) lcd.write(byte(0));
    else if (direccionAscensor == -1) lcd.write(byte(1));
  } else {
    lcd.print(plantaActual);
  }

  lcd.print(" L:");
  lcd.print(brillo);
  lcd.print("   "); 

  // --- SEGUNDA LÍNEA ---
  lcd.setCursor(0, 1);
  
  // 1. Alerta Prioritaria: Mantenimiento (Mando IR)
  if (modoMantenimiento) {
    lcd.print("EN MANTENIMIENTO");
    return; // Sale de la función para no sobreescribir el texto
  }
  
  // 2. Alerta Secundaria: Fallo del Sensor (Autodiagnóstico)
  if (errorSensores) {
    lcd.print("ERR SENSOR DHT22");
    return; // Sale de la función
  }

  // 3. Operación Normal: Muestra Temperatura y Humedad
  lcd.print("T:");
  lcd.print((int)temperatura);
  if (estadoTemp == 0) lcd.print("OK");
  else if (estadoTemp == 1) lcd.write(byte(0));
  else lcd.write(byte(1));

  lcd.print(" H:");
  lcd.print((int)humedad);
  if (estadoHum == 0) lcd.print("OK");
  else if (estadoHum == 1) lcd.write(byte(0));
  else lcd.write(byte(1));
  lcd.print("  ");
}

void telemetriaYLog(int brillo, float t, float h) {
  // 1. Envío por Bus Industrial simulado (Serial1) de forma directa
  Serial1.print("{\"P\":"); Serial1.print(plantaActual);
  Serial1.print(",\"T\":"); 
  if (errorSensores) Serial1.print("null"); else Serial1.print(t);
  Serial1.print(",\"H\":"); 
  if (errorSensores) Serial1.print("null"); else Serial1.print(h);
  Serial1.print(",\"L\":"); Serial1.print(brillo);
  Serial1.print(",\"Err\":"); Serial1.print(errorSensores);
  Serial1.println("}");

  // Monitorización local (Serial)
  Serial.print("TX: {\"P\":"); Serial.print(plantaActual);
  Serial.print(",\"T\":"); 
  if (errorSensores) Serial.print("null"); else Serial.print(t);
  Serial.print(",\"H\":"); 
  if (errorSensores) Serial.print("null"); else Serial.print(h);
  Serial.print(",\"L\":"); Serial.print(brillo);
  Serial.print(",\"Err\":"); Serial.print(errorSensores);
  Serial.println("}");

  // 2. Registro en Tarjeta SD (Caja Negra)
  File dataFile = SD.open("datalog.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.print(millis()); dataFile.print(",");
    dataFile.print(plantaActual); dataFile.print(",");
    dataFile.print(t); dataFile.print(",");
    dataFile.print(h); dataFile.print(",");
    dataFile.print(brillo); dataFile.print(",");
    dataFile.println(errorSensores ? "FALLO" : "OK");
    dataFile.close();
  } else {
    Serial.println("Error abriendo datalog.csv");
  }
}