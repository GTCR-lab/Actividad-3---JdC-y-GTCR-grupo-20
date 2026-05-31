#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// 1. LIBRERÍAS
// ==========================================
#include <Servo.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include <IRremote.h>

// ==========================================
// 2. DEFINICIÓN DE PINES (HARDWARE)
// ==========================================
// Botonera ascensor
const int pinBoton1 = 3;
const int pinBoton2 = 4;
const int pinBoton3 = 5;
const int pinBoton4 = 6;
const int pinBoton5 = 7;

// Sensores y actuadores principales
const int servoPin = 10;
const int dhtPin = 12;
const int trigPin = 8;
const int echoPin = 9;

// Indicadores LED (Climatización e Iluminación)
const int ledAzul = 2;
const int ledRojo = A1;
const int ledMorado = A2;
const int ledAmarillo = A3;
const int ledLuz = 11;

// Buses y comunicaciones
const int chipSelect = 53; // CS para MicroSD (Bus SPI)
const int irPin = 40;      // Receptor IR

// ==========================================
// 3. CONSTANTES DEL SISTEMA
// ==========================================
#define DHTTYPE DHT22
const long intervaloLog = 5000; // Enviar datos cada 5 segundos

#endif