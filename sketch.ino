#include "config.h"

// ==========================================
// VARIABLES DINÁMICAS Y OBJETOS
// ==========================================
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

// Variables Seguridad y Eficiencia
bool modoMantenimiento = false;
bool modoEco = false;

// Temporizadores Telemetría
unsigned long tiempoUltimoLog = 0;

// Instancias de Objetos
DHT dht(dhtPin, DHTTYPE);
Servo servo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Caracteres personalizados LCD
byte flechaArriba[8] = { B00100, B01110, B10101, B00100, B00100, B00100, B00100, B00000 };
byte flechaAbajo[8] = { B00100, B00100, B00100, B00100, B10101, B01110, B00100, B00000 };

// =================================================================================
// SETUP
// =================================================================================
void setup() {
  Serial.begin(9600);  // Serial 0: Depuración
  Serial1.begin(9600); // Serial 1: Bus Industrial RS-485

  Serial.println("Iniciando Sistema ACME S.A...");

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

  servo.attach(servoPin);
  servo.write(0);
  dht.begin();
  
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, flechaArriba);
  lcd.createChar(1, flechaAbajo);

  IrReceiver.begin(irPin, ENABLE_LED_FEEDBACK);
  
  Serial.print("Inicializando tarjeta SD... ");
  pinMode(chipSelect, OUTPUT); 
  delay(1000); 

  if (!SD.begin(chipSelect)) {
    Serial.println("Fallo o tarjeta no presente.");
  } else {
    Serial.println("Tarjeta inicializada OK.");
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
  controlarIR(); 
  
  if (!enMovimiento) {
    if (digitalRead(pinBoton1) == LOW) {MoverAscensor(1, 0);}
    else if (digitalRead(pinBoton2) == LOW) {MoverAscensor(2, 45);}
    else if (digitalRead(pinBoton3) == LOW) {MoverAscensor(3, 90);}
    else if (digitalRead(pinBoton4) == LOW) {MoverAscensor(4, 135);}
    else if (digitalRead(pinBoton5) == LOW) {MoverAscensor(5, 180);}
  }

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

  int brillo;
  float temperatura, humedad;
  
  controlarIluminacion(brillo);
  leerAmbiente(temperatura, humedad);
  autodiagnostico(temperatura, humedad);
  actualizarLCD(brillo, temperatura, humedad);
  
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
    
    // 1. PARADA DE EMERGENCIA / MANTENIMIENTO
    if (comando == 162) { // Botón POWER
      modoMantenimiento = !modoMantenimiento;
      Serial.println(modoMantenimiento ? "MODO MANTENIMIENTO ACTIVADO" : "SISTEMA OPERATIVO");
    }

    // 2. CONTROLES OPERATIVOS
    if (!modoMantenimiento && !enMovimiento) {
      switch (comando) {
        // Llamadas a plantas (Botones 1-5 del mando)
        case 48: MoverAscensor(1, 0); break;   
        case 24: MoverAscensor(2, 45); break;  
        case 122: MoverAscensor(3, 90); break; 
        case 16: MoverAscensor(4, 135); break; 
        case 56: MoverAscensor(5, 180); break; 
        
        // Ajuste de Temperatura (Reemplaza los números si cambiaste de botones)
        case 168: setpointTemp++; Serial.print("Temp Obj: "); Serial.println(setpointTemp); break; // VOL+
        case 224: setpointTemp--; Serial.print("Temp Obj: "); Serial.println(setpointTemp); break; // VOL-
        
        // Ajuste de Humedad
        case 144: setpointHum += 5; Serial.print("Hum Obj: "); Serial.println(setpointHum); break; // >>
        case 208: setpointHum -= 5; Serial.print("Hum Obj: "); Serial.println(setpointHum); break; // <<
        
        // Perfil Energético
        case 34: // Botón TEST
          modoEco = !modoEco; 
          zonaMuertaTemp = modoEco ? 5.0 : 2.0; 
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

  if (errorSensores || modoMantenimiento) {
    digitalWrite(ledRojo, LOW);
    digitalWrite(ledAzul, LOW);
    digitalWrite(ledMorado, LOW);
    digitalWrite(ledAmarillo, LOW);
    return; 
  }

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
  if (isnan(t) || isnan(h)) {
    errorSensores = true;
  } else {
    errorSensores = false;
  }
}

void actualizarLCD(int brillo, float temperatura, float humedad) {
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

  lcd.setCursor(0, 1);
  
  if (modoMantenimiento) {
    lcd.print("EN MANTENIMIENTO");
    return; 
  }
  
  if (errorSensores) {
    lcd.print("ERR SENSOR DHT22");
    return; 
  }

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
  Serial1.print("{\"P\":"); Serial1.print(plantaActual);
  Serial1.print(",\"T\":"); 
  if (errorSensores) Serial1.print("null"); else Serial1.print(t);
  Serial1.print(",\"H\":"); 
  if (errorSensores) Serial1.print("null"); else Serial1.print(h);
  Serial1.print(",\"L\":"); Serial1.print(brillo);
  Serial1.print(",\"Err\":"); Serial1.print(errorSensores);
  Serial1.println("}");

  Serial.print("TX: {\"P\":"); Serial.print(plantaActual);
  Serial.print(",\"T\":"); 
  if (errorSensores) Serial.print("null"); else Serial.print(t);
  Serial.print(",\"H\":"); 
  if (errorSensores) Serial.print("null"); else Serial.print(h);
  Serial.print(",\"L\":"); Serial.print(brillo);
  Serial.print(",\"Err\":"); Serial.print(errorSensores);
  Serial.println("}");

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
