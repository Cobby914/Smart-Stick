#include <Arduino.h>
#include "Secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// --- AWS OBJECTS ---
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

// --- BLUETOOTH & SERIAL ---
#define RXD2 16
#define TXD2 17
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

// --- CONNECT TO AWS FUNCTION ---
void connectAWS() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Configure Certificates
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  client.setServer(AWS_IOT_ENDPOINT, 8883);

  Serial.println("\nConnecting to AWS IoT...");
  while (!client.connected()) {
    if (client.connect("SmartStick_ESP32")) {
      Serial.println("AWS Connected!");
    } else {
      Serial.print("Failed. State: ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

// --- BLUETOOTH CALLBACKS ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; };
    void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        String message = String(value.c_str());
        Serial2.print(message);
        Serial2.print('\n'); 
      }
    }
};

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // 1. Connect to AWS
  connectAWS();

  // 2. Setup Bluetooth (Same as before)
  BLEDevice::init("Smart Stick");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE
                    );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); 
  BLEDevice::startAdvertising();
}

void loop() {
  // 1. Keep AWS Connection Alive
  if (!client.connected()) {
    connectAWS();
  }
  client.loop();

  // 2. Check for Data from Arduino
  if (Serial2.available()) {
    // Read the "1,0,1" string
    String message = Serial2.readStringUntil('\n');
    message.trim(); // Clean up whitespace

    // 3. Send to AWS Cloud (The Magic Line)
    // Topic: "smartstick/sensors"
    // Payload: "1,0,1"
    client.publish("smartstick/sensors", message.c_str());

    // Debug: Print to your laptop so you know it happened
    Serial.print("Sent to Cloud: ");
    Serial.println(message);

    // 4. (Optional) Also send to Phone via Bluetooth
    if (deviceConnected) {
      pCharacteristic->setValue(message.c_str());
      pCharacteristic->notify();
    }
  }
}