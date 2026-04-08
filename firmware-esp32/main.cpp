#include <WiFi.h>
#include <PubSubClient.h>

// --- CONFIGURACIÓN WI-FI ---
const char* ssid = "***";
const char* password = "***";

// --- CONFIGURACIÓN MQTT ---
const char* mqttServer = "3.150.188.167";
const int mqttPort = 1883;

// --- TÓPICO MQTT ---
const char* topicFlujo = "sensor/flujo";

// --- PINES ---
#define FLUJO_PIN 25

// --- VARIABLES ---
volatile int pulsos = 0;
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastPublish = 0;
const unsigned long intervalo = 2000;

void IRAM_ATTR contarPulso() {
  pulsos++;
}

void conectarMQTT() {
  while (!client.connected()) {
    Serial.println("Conectando a MQTT...");
    if (client.connect("ESP32FlujoAgua")) {
      Serial.println("Conectado al broker MQTT");
    } else {
      Serial.print("Falló, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(FLUJO_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLUJO_PIN), contarPulso, RISING);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");

  client.setServer(mqttServer, mqttPort);
  conectarMQTT();
}

void loop() {
  if (!client.connected()) conectarMQTT();
  client.loop();

  unsigned long now = millis();
  if (now - lastPublish >= intervalo) {
    lastPublish = now;

    noInterrupts();
    int pulsosMedidos = pulsos;
    pulsos = 0;
    interrupts();

    float frecuencia = pulsosMedidos / (intervalo / 1000.0);
    float flujo = frecuencia / 7.5;

    char flujoStr[10];
    dtostrf(flujo, 1, 2, flujoStr);
    client.publish(topicFlujo, flujoStr);

    Serial.print("Flujo (L/min): ");
    Serial.println(flujo, 2);
  }
}