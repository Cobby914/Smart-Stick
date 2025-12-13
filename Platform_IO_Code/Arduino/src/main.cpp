#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"

// --- PINS ---
const int trig1 = 2, echo1 = 3;  // Left
const int trig2 = 4, echo2 = 5;  // Right
// const int buzz1 = 8;             // Left Motor
// const int buzz2 = 9;             // Right Motor
// const int buzz3 = 10;            // Center Motor

// --- SENSOR OBJECT ---
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

int WALL_LIMIT_CM = 30;  // Default: Alert if closer than 30cm
int DIP_LIMIT_MM = 600;  // Default: Alert if floor is > 600mm away

void setup() {
  Serial.begin(9600); // Communicate with ESP32

  // Pin Setup
  pinMode(trig1, OUTPUT); pinMode(echo1, INPUT);
  pinMode(trig2, OUTPUT); pinMode(echo2, INPUT);
  // pinMode(buzz1, OUTPUT); pinMode(buzz2, OUTPUT); pinMode(buzz3, OUTPUT);

  // Initialize ToF
  if (!lox.begin()) {
    while(1); // Freeze if sensor missing
  }
}

// Helper to read HC-SR04
long readUltrasonic(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  return pulseIn(echo, HIGH) / 58; // Convert to cm
}

void loop() {
  // --- 1. LISTEN FOR COMMANDS FROM PHONE ---
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Clean up whitespace
    
    char type = command.charAt(0);
    int value = command.substring(1).toInt();

    if (type == 'W') { 
      WALL_LIMIT_CM = value; 
    }
    else if (type == 'D') { 
      DIP_LIMIT_MM = value; 
    }
  }

  // --- 2. READ SENSORS ---
  long leftCM = readUltrasonic(trig1, echo1);
  long rightCM = readUltrasonic(trig2, echo2);
  
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);
  int centerMM = (measure.RangeStatus != 4) ? measure.RangeMilliMeter : 9999;

  // --- 3. CALCULATE ALERT STATES (1 = Danger, 0 = Safe) ---
  int leftState = (leftCM > 0 && leftCM < WALL_LIMIT_CM) ? 1 : 0;
  int rightState = (rightCM > 0 && rightCM < WALL_LIMIT_CM) ? 1 : 0;
  int centerState = (centerMM > DIP_LIMIT_MM) ? 1 : 0; // Dip Logic

  // // --- 4. CONTROL BUZZERS ---
  //TRIED: tried to implment this but our buzzers pins do not connect to our breadboard properly
  // if (leftState) digitalWrite(buzz1, HIGH); else digitalWrite(buzz1, LOW);
  // if (rightState) digitalWrite(buzz2, HIGH); else digitalWrite(buzz2, LOW);
  // if (centerState) digitalWrite(buzz3, HIGH); else digitalWrite(buzz3, LOW);

  // --- 5. SEND DATA TO ESP32 ---
  Serial.print(leftState); Serial.print(",");
  Serial.print(centerState); Serial.print(",");
  Serial.println(rightState);

  delay(200); // 5 updates per second
}